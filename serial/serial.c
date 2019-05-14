
#include <unistd.h>
#include "serial.h"

const char* serialPorts[5]= {	ttyACM0,
								ttyACM1,
								ttyACM2,
								ttyACM3,
								ttyACM4
							};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

int openSerialCommunication(int* fd){
	printf("\nSearch for AVR device on serial ports...\n");
	int k = 0;
	for ( ; k<5 ; k++){
		*fd = open(serialPorts[k], O_RDWR | O_NOCTTY | O_NDELAY);
		if (*fd >= 0){
			printf("# /dev/ttyACM%d found.\n", k);
			tcflush(*fd, TCIOFLUSH);
			return k;
		}
		else printf("# /dev/ttyACM%d not found\n", k);
	}
	return -1;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void setSerialAttributes(int fd){

	fcntl(fd, F_SETFL, 0); //set read as blocking

	struct termios serial_settings;

	tcgetattr(fd, &serial_settings);	// Get the current attributes of the Serial port

	cfsetispeed(&serial_settings,B19200); // Set Read  Speed as 9600
	cfsetospeed(&serial_settings,B19200); // Set Write Speed as 9600

	serial_settings.c_cflag &= ~PARENB;   // Disables the Parity*/
	serial_settings.c_cflag &= ~CSTOPB;   // CSTOPB = 2 Stop bits, here it is cleared so 1 Stop bit*/
	serial_settings.c_cflag &= ~CSIZE;	 // Clears the mask for setting the data size             */
	serial_settings.c_cflag |=  CS8;      // Set the data bits = 8

	serial_settings.c_cflag &= ~CRTSCTS;   // Disable Hardware flow Control                         */
	serial_settings.c_cflag |= CREAD | CLOCAL; // Enable receiver

	serial_settings.c_iflag &= ~(IXON | IXOFF | IXANY);	// Disable XON/XOFF flow control
	serial_settings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Non Cannonical mode

	serial_settings.c_oflag &= ~OPOST;	//No Output Processing
	
	/* read at least this bytes */
	serial_settings.c_cc[VMIN] = 3;

	/* no minimum time to wait before read returns */
	serial_settings.c_cc[VTIME] = 0;

	tcflush(fd, TCIOFLUSH);
	if((tcsetattr(fd, TCSANOW, &serial_settings)) != 0){
		printf("\n ERROR! cannot set serial attributes\n\n");
		exit(EXIT_FAILURE);
	}
	else{
		printf("\n  | BaudRate = %d \n  | StopBits = 1 \n  | Parity   = none\n\n", BAUD_RATE);
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void closeSerialCommunication(int* fd){

	tcflush(*fd, TCIOFLUSH);
	int ret = close(*fd);
	if(ret != 0) {
		printf("\n close(fd) syscall failed [%d]\n", ret);
		exit(EXIT_FAILURE);
	}
}
