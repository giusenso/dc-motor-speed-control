#include <util/delay.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "avr_common/uart.h"

#define BAUD 19200
#define MYUBRR (F_CPU/16/BAUD-1)

#define TCCRA_MASK	(1<<WGM11)|(1<<COM1A1)|(1<<COM1B1);	//NON Inverted PWM
#define	TCCRB_MASK	(1<<WGM13)|(1<<WGM12)|(1<<CS10);	  //FAST PWM with NO

#define		CWISE	0x00
#define		CCWISE	0xFF	


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

/*::::: M A I N :::::::::::::::::::::::::::::::::::::::::*/
/*    
*   This is a basic motor test. Motor speed go up and down
*   and the direction switch every time the motor stop
*/
int main(void){
  
  PWM_init();
  _delay_ms(1000);

  uint8_t speed = 0, dir = CWISE;
  setSpeed( speed );
  setDirection( CWISE );

  while (true){

    while( speed<255 ){
      speed++;
      setSpeed(speed);
      _delay_ms(30);
    }

    while( speed>0 ){
      speed--;
      setSpeed(speed);
      _delay_ms(30);
    }

    if( dir==CWISE ) dir=CCWISE;
    else if( dir==CCWISE ) dir=CWISE;
    setDirection( dir );
  }
}

/*::: INTERRUPT SERVICE ROUTINES ::::::::::::::::::::::::*/

//...

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::*/