# DC Motor speed control using atmega2560 and H-bridge #

~ PC to MCU serial communication to set the motor parameters
~ MCU to PC serial communication to check the motor status

On MCU side:
- PWM generation
- Periodically send status-packets with a timer based ISR
- Receive setting-packets with UART based ISR

On PC side:
- Open, set and close serial communication using unix API
- Multithreading (1 sender thread, 1 listener thread)
- Graphic Unit Interface with ncurses


# 10.05.19
	- design overall software workflow
	- packet data structure
	- PWM generation: motor now work properly
	
# 11.05.19
	- UART basic functions
	
# 12.05.19
	- timer based ISR (send status every 1 second)
	- UART ISR (reading incoming packet)
	- Handshaking routine
	- test new features: works as expected (with cutecom)

# 13.05.19
	- ...
