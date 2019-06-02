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
#define	TCCRB_MASK	(1<<WGM13)|(1<<WGM12)|(1<<CS10);	//FAST PWM with NO

#define		CWISE	     		0xAA
#define		CCWISE      		0xBB
#define   	OS_FLAG     		'>'		// open serial flag
#define   	CS_FLAG     		'<'		// close serial flag
#define   	MIN_SPEED   		100
#define   	MAX_SPEED			200
#define   	OCR_TOP_VALUE		39899
#define   	ONE_PERCENT_STEP  	OCR_TOP_VALUE/100 //~400

//---------------------------------------------------------
void UART_init(void){
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
uint8_t UART_getString(uint8_t* buf){
	uint8_t* b0 = buf; //beginning of buffer
	while(1){
		uint8_t c = UART_getChar();
		*buf = c;
		++buf;
		// reading a 0 terminates the string
		if( c==0 ) return buf-b0;
		// reading a \n  or a \r return results
    	// in forcedly terminating the string
		if( c=='\n' || c=='\r' ){
			*buf = 0;
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
//--------------------------------------------------------

void PWM_init(void){
	/* Timer 3 = digital pin 5 = DDRE */
  	//Data direction register
	//DDRE |= 0xFF;   	//digital pin 5 ON
	DDRE &= 0x00;   	//digital pin 5	OFF

	//Configure TIMER3
	TCCR3A = TCCRA_MASK;
	TCCR3B = TCCRB_MASK;

	//Top timers value
	ICR3 = 39999;
}

void PWM_start(void){
	DDRE |= 0xFF; 
}

void PWM_stop(void){
	DDRE &= 0x00;
}

//---------------------------------------------------------

void setupTimer(void){
	// set the prescaler to 1024
	TCCR5A = 0x00;
	TCCR5B = (1 << WGM52) | (1 << CS50) | (1 << CS52);
	TIMSK5 |= (1 << OCIE5A);  // enable the timer interrupt
	OCR5A = F_CPU/1024-1;
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
	OCR3A = (speed-MIN_SPEED)*ONE_PERCENT_STEP;
}

/* using linear interpolation */
void set_speed_smoothly(uint8_t speed){
	uint16_t delay = 1000;
	uint8_t num_step = 20;
	uint8_t delay_per_step = delay/num_step;
	uint16_t new_ocr = (speed-MIN_SPEED)*ONE_PERCENT_STEP;

	if( new_ocr>OCR3A ){
		uint16_t step = (new_ocr-OCR3A)/num_step;
		for(uint8_t i=1 ; i<=num_step ; i++){
			OCR3A += step;
			_delay_ms(delay_per_step);
		}
	}
	else if( new_ocr<OCR3A ){
		uint16_t step = (OCR3A-new_ocr)/num_step;
		for(uint8_t i=0 ; i<num_step ; i++){
			OCR3A -= step;
			_delay_ms(delay_per_step);
		} 
	}
	setSpeed(speed);
}

//---------------------------------------------------------

bool packetMatch(uint8_t* p, uint8_t c){
	if( p[0]==c && p[1]-100==c && p[2]==c)return true;
	else return false;
}

//---------------------------------------------------------


//---------------------------------------------------------
/*  global variables  */
volatile uint8_t timer_occurred = false;
volatile uint8_t msg_rcv = false;

/*=======================================================*/
/*::::: M A I N :::::::::::::::::::::::::::::::::::::::::*/

int main(void){
	cli();

	uint8_t buf[4], hs = 0;
	uint8_t _packet_rate = 1;
	uint8_t _timestamp = 1;
	uint8_t _speed = MIN_SPEED;
	uint8_t _direction = CWISE;

	bool running = false;
	UART_init();
	_delay_ms(1000);
	
	// handshake routine ----------------------------------
	uint8_t hshake[4] = {OS_FLAG,OS_FLAG+101,OS_FLAG+2,10};
	if( hs==0 ){
		
		UART_putChar(hshake[0]);
		UART_putChar(hshake[1]);
		UART_putChar(hshake[2]);
		UART_putChar(hshake[3]);
		hs = 1;
		sei();
	}
	while ( !running ){
		if( msg_rcv && hs==1 ){
			cli();
			UART_getString(hshake);
			UART_putChar(hshake[0]);
			UART_putChar(hshake[1]);
			UART_putChar(hshake[2]);
			UART_putChar(hshake[3]);
			hs = 2;
			msg_rcv = false;
			sei();
		}
		if( msg_rcv && hs==2 ){
			cli();
			UART_getString(hshake);
			UART_putChar(hshake[0]);
			UART_putChar(hshake[1]);
			UART_putChar(hshake[2]);
			UART_putChar(hshake[3]);
			hs = 3;
			running = true;
			msg_rcv = false;
			sei();
		}
		continue;
	}
	//-------------------------------------------------
	cli();

		// timer used as interrupt trigger
		setupTimer();
		
		// pwm to control motor speed 
		PWM_init();

		// set starting parameters
		setSpeed(_speed);
		setDirection(_direction);
		setPacketRate(1);
		PWM_start();

		sei();
		
		// MAIN loop ------------------------------------------

		while( running ){

			if( timer_occurred ){
				UART_putChar(_timestamp);
				UART_putChar(_speed);
				UART_putChar(_direction);
				UART_putChar(10);
				_timestamp = _timestamp<255?_timestamp+1:1;
				timer_occurred = false;
			}     

			if( msg_rcv ){
				UART_getString(buf);				
				if( buf[0]==CS_FLAG ){
					cli();
					running = false;
				}		
				else{
					if( _packet_rate!=buf[0] ){
						_packet_rate = buf[0];
						setPacketRate(_packet_rate);
					}
					if( _speed!=buf[1]){
						_speed = buf[1];
						setSpeed(_speed);
					}
					if( _direction != buf[2] ){
						_direction = buf[2];
						set_speed_smoothly(MIN_SPEED);
						setDirection(_direction);
						set_speed_smoothly(_speed);
					}
				}
				msg_rcv = false;   
			}
		}

		PWM_stop();
		_timestamp = 1;
		_packet_rate = 1;
		setPacketRate(_packet_rate);
		_speed = MIN_SPEED;
		setSpeed(_speed);
		buf[0] = buf[1] = buf[2] = buf[3] = 0;
		_delay_ms(3000);
		
		//-------------------------------------------------
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
