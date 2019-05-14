#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include "avr_common/uart.h"

#define BAUD 19200
#define MYUBRR (F_CPU/16/BAUD-1)

#define TCCRA_MASK	(1<<WGM11)|(1<<COM1A1)|(1<<COM1B1);	//NON Inverted PWM
#define	TCCRB_MASK	(1<<WGM13)|(1<<WGM12)|(1<<CS10);	  //FAST PWM with NO

#define     CWISE		    0xAA
#define     CCWISE      0xBB
#define   	OS_FLAG     0x3E  // > opens serial flag
#define   	CS_FLAG     0x3C  // < close serial flag

//---------------------------------------------------------
void UART_init(void){
  // Set baud rate
  UBRR0H = (uint8_t)(MYUBRR>>8);
  UBRR0L = (uint8_t)MYUBRR;

  UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);               /* 8-bit data */
  UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);   /* Enable RX and TX */
}

//---------------------------------------------------------
void UART_putChar(uint8_t c){
  // wait for transmission completed, looping on status bit
  while ( !(UCSR0A & (1<<UDRE0)) );

  // Start transmission
  UDR0 = c;
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

volatile uint8_t timer_occurred = 0;
volatile uint8_t msg_rcv = 0;

/*=======================================================*/
/*::::: M A I N :::::::::::::::::::::::::::::::::::::::::*/

int main(void){
  cli();

  PWM_init();
  setSpeed( 0 );
  setDirection( CWISE );

  setupTimer();
  OCR5A = F_CPU/1024-1;

  bool running = false;
  UART_init();

  uint8_t buf[4]={0xFF,0xFF,0xFF,0};

  //handshake routine -------------------------------------
  while(!running){

	//[ ?, ?, OS_FLAG, 0]
    uint8_t hshake[4] = {0xFF, 0xFF, OS_FLAG, 0};

    UART_putString(hshake);

    UART_getString(hshake);
    if( hshake[0]==OS_FLAG ){
      buf[0] = 1;
      buf[1] = hshake[1];
      buf[2] = hshake[2];
      setSpeed(buf[1]);
      setDirection(buf[2]);
      running=true;
    }
  }
  //-------------------------------------------------------

  sei();
  while( running ){

    if( timer_occurred ){
      UART_putString((uint8_t*)buf);
      buf[0]++;
      timer_occurred = false;
    }

    if( msg_rcv ){
      cli();
      UART_getString((uint8_t*)buf);
      if( buf[0]==CS_FLAG ){
        running = false;
      }
	  }
    else{
		  //setPacketRate(buf[0]);
      setSpeed(buf[1]);
      setDirection(buf[2]);
      msg_rcv = false;
    }
		sei();
	}
	continue;

	//exit procedure: send[?, ?, CS_FLAG, 0]
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
