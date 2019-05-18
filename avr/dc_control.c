#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "avr_common/uart.h"

#define BAUD 19200
#define MYUBRR (F_CPU/16/BAUD-1)

#define TCCRA_MASK	(1<<WGM11)|(1<<COM1A1)|(1<<COM1B1);	//NON Inverted PWM
#define	TCCRB_MASK	(1<<WGM13)|(1<<WGM12)|(1<<CS10);	  //FAST PWM with NO

#define		CWISE	      0xAA
#define		CCWISE      0xBB
#define   OS_FLAG     0x3E  // > opens serial flag
#define   CS_FLAG     0x3C  // < close serial flag

//---------------------------------------------------------
void UART_init(void){
  // Set baud rate
  UBRR0H = (uint8_t)(MYUBRR>>8);
  UBRR0L = (uint8_t)MYUBRR;

  UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);               /* 8-bit data */ 
  UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);   /* Enable RX and TX */  
}

//---------------------------------------------------------
uint8_t UART_getChar(void){
  // Wait for incoming data, looping on status bit
  while ( !(UCSR0A & (1<<RXC0)) );

  // Return the data
  return UDR0;
}

//----------------------------------------------------------
// reads a string until the first newline or 0
// returns the size read
uint8_t UART_getString(uint8_t* buf){
  uint8_t* b0=buf; //beginning of buffer
  while(1){
    uint8_t c=UART_getChar();
    *buf=c;
    ++buf;
    // reading a 0 terminates the string
    if (c==0)
      return buf-b0;
    // reading a \n  or a \r return results
    // in forcedly terminating the string
    if(c=='\n'||c=='\r'){
      *buf=0;
      ++buf;
      return buf-b0;
    }
  }
}

//---------------------------------------------------------
void UART_putChar(uint8_t c){
  // wait for transmission completed, looping on status bit
  while ( !(UCSR0A & (1<<UDRE0)) );

  // Start transmission
  UDR0 = c;
}

//---------------------------------------------------------
void UART_putString(uint8_t* buf){
  while(*buf){
    UART_putChar(*buf);
    ++buf;
  }
}

//---------------------------------------------------------
void PWM_init(void){
  /* Timer 3 = digital pin 5 = DDRE */

  //Data direction register
  DDRE |= 0xFF;    //digital pin 5

  //Configure TIMER3
  TCCR3A = TCCRA_MASK;
  TCCR3B = TCCRB_MASK;

  //Top timers value
  ICR3 = 39999;

}

//---------------------------------------------------------

void setupTimer(void){
  // set the prescaler to 1024
  TCCR5A = 0x00;
  TCCR5B = (1 << WGM52) | (1 << CS50) | (1 << CS52);
  TIMSK5 |= (1 << OCIE5A);  // enable the timer interrupt
}

//---------------------------------------------------------

void setPacketRate(uint8_t _packet_per_sec){
  if(!_packet_per_sec){ /*do something*/ }
  OCR5A = (uint16_t)((F_CPU/1024/_packet_per_sec)-1);
}

//---------------------------------------------------------

void setDirection(uint8_t dir){	
	if( dir==CWISE ){
	  PORTH &= ~(1 << PH6);	//digital pin 9 low
		PORTH |= (1 << PH5);	//digital pin 8 high
	}
	else if( dir==CCWISE ){
		PORTH &= ~(1 << PH5);	//digital pin 8 low
		PORTH |= (1 << PH6);	//digital pin 9 high
	}
}

//---------------------------------------------------------
void setSpeed(uint8_t speed){
	OCR3A = speed * 149 + 2000;
}

//---------------------------------------------------------
/*  global variables  */

volatile uint8_t timer_occurred = false;
volatile uint8_t msg_rcv = false;

//uint8_t speed, direction, packet_rate;

/*=======================================================*/
/*::::: M A I N :::::::::::::::::::::::::::::::::::::::::*/

int main(void){
  cli();

  PWM_init();

  setupTimer();
  OCR5A = F_CPU/1024-1;

  setSpeed(1);
  setDirection(CWISE);

  bool running = false;
  UART_init();

  uint8_t buf[4];
  uint8_t _timestamp = 1;
  uint8_t _speed = 1;
  uint8_t _direction = CWISE;
  uint8_t _packet_rate = 1;

  while(true){// infinite loop ----------------------------

    // handshake routine ----------------------------------
    while( !running ){
      uint8_t hshake[3] = { 97, 98, OS_FLAG };
      UART_putChar(97);
      UART_putChar(98);
      UART_putChar(OS_FLAG);
      UART_putChar(10);

      UART_getString(hshake);
      if( hshake[0]==OS_FLAG ){
        _timestamp = 0;
        _packet_rate = 1;
        _speed = hshake[1];
        _direction = hshake[2];
        running = true;
        break;
      }
    }
    
    setSpeed(_speed);
    setDirection(_direction);
    _delay_ms(1010);

    // MAIN loop ------------------------------------------
    sei();
    while( running ){

      if( timer_occurred ){
        UART_putChar(_timestamp);
        UART_putChar(_speed);
        UART_putChar(_direction);
        UART_putChar(10);
        _timestamp = _timestamp<=255 ? _timestamp+1 : 1;
        timer_occurred = false;
      }     
      
      if( msg_rcv ){
        UART_getString((uint8_t*)buf);
        if( buf[0]==CS_FLAG ){
          cli();
          running = false;
        }
        else{
          _packet_rate = buf[0];
          _speed = buf[1];
          _direction = buf[2];
          //setPacketRate(_packet_rate);
          setSpeed(_speed-100);
          setDirection(_direction);
        }
        msg_rcv = false;   
      }
    }
    continue;
  }
}

/*=======================================================*/
/*::: INTERRUPT SERVICE ROUTINES ::::::::::::::::::::::::*/

ISR(TIMER5_COMPA_vect) {
  timer_occurred = true;
}

ISR(USART0_RX_vect) {
  msg_rcv = true;
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::*/



