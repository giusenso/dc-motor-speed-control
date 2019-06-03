# DC Motor Speed Controller

## Table of contents
* [Project description](#Project-description)
* [Hardware](#Hardware)
* [Technologies](#Technologies)
* [How to run](#How-to-run)
* [Development](#Development)

## Project description
The main purpose of this project is to control DC motors in open loop, in order to set speed and direction in real time.
One MCU and one H-bridge are used to achieve this. PC and MCU communicates writing each other custom packets.
Basically, the idea is this:
* PC to MCU serial communication to set the motor parameters.
* MCU to PC serial communication to check the motor status.

### MCU side
* PWM generation
* Periodically send status-packets with a timer based ISR
* Receive setting-packets with UART based ISR

### PC side
* Open, set and close serial communication using unix API
* Multithreading (1 sender thread, 1 listener thread)
* Graphic Unit Interface with ncurses
* Option flags handling

(Other info can be found in the repo's wiki.)

## Hardware
* AVR atmega2560
* DC motor
* Battery pack (6V, 2850mAh)
* L298N H-bridge

## Technologies
* OS: Ubuntu 16.04
* Language: C
* PC compiler: gcc (version 5.4.0)
* MCU compiler: avr-gcc (version 4.9.2)
* GUI lib: ncurses.h


## How to run

### Installation
Firts of all, make sure you have installed ncurses library. If not, type:
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
```

To upload AVR code in your microcontroller you need avr-gcc:
```bash
sudo apt-get install gcc-avr binutils-avr gdb-avr avr-libc avrdude
```

AVR code can be compiled and uploaded typing :
```bash
cd avr
make
make dc_control.hex
sudo addgroup <your username> dialout
cd ..
```

Pc-side code can be found in "serial" directory. Run Makefile in order to compile and create the executable:
```bash
cd serial
make
```

After that, you are ready to run the program.

### Usage
Pc-side code can be found in "serial" directory. Run Makefile in order to compile and create the executable:
```bash
./main
```
Main can be launched with different optional flags:

```bash
./main -l
```
Enable linear interpolation on avr. With this flag enabled motor direction change smoothly.

```bash
./main -f
```
Force avr re-upload/restart.

```bash
./main -d
```
Debuf mode (deprecated). Used for communication testing, GUI disabled.


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
	
### 03.06.19
	- cleaning: remove useless stuff
	- fixing linear interpolation
