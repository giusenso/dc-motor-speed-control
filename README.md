# DC Motor speed control using atmega2560 and H-bridge

## Table of contents
* [Project description](#the-project)
* [Hardware](#hardware)
* [Technologies](#technologies)
* [Features](#features)
* [Development](#development)

## The project
The purpose of this project is to control a DC motor in open loop from a PC. A microcontroller generate a pwm to move the motor. Motor's speed and direction is acquired by serial. PC side there is an easy graphic unit interface to set the parameters.


## Hardware
* AVR atmega2560
* Dc motor (6V)
* Battery pack (6V, 2850mAh)
* L298N H-bridge

## Technologies

* OS: Ubuntu 16.04
* Language: C
* PC compiler: gcc (version 5.4.0)
* MCU compiler: avr-gcc (version 4.9.2)
* GUI lib: ncurses.h

## Features
* PC to MCU serial communication to set the motor parameters
* MCU to PC serial communication to check the motor status

### MCU side

* PWM generation
* Periodically send status-packets with a timer based ISR
* Receive setting-packets with UART based ISR

### PC side
* Open, set and close serial communication using unix API
* Multithreading (1 sender thread, 1 listener thread)
* Graphic Unit Interface with ncurses


## Development

### 10.05.19
	- design overall software workflow
	- packet data structure
	- PWM generation: motor now work properly
	
### 11.05.19
	- UART basic functions
	
### 12.05.19
	- timer based ISR (send status every 1 second)
	- UART ISR (reading incoming packet)
	- handshaking routine
	- test new features: works as expected (with cutecom)

### 14.05.19
	- start working on pc side
	- set termios struct parameters
	- read() and write() syscall
	- makefile

### 18.05.19
	- transmission fixes (avr side)
	- handshake protocol
	- serial packet_t structure
	- serial functions (readPacket() and writePacket())
	- packet_t data manipulation functions
	
### 19.05.19
	- fixes on avr side
	- fixes on readPacket() and writePacket()
	- listener thread for blocking read
	- mutex synch
	- main testing: some bugs, but works

### 23.05.19
	- fix bug #2
	- linear interpolation for smooth speed changes

### 01-02.06.19
	- testing different handshake protocols
	- choose the definitive handshake
	- add option flag -s to enable linear interpolation on avr
	- add option flag -f to force avr re-upload
