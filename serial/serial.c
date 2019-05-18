
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
			printf("\n# /dev/ttyACM%d found.\n", k);
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

	struct termios serial_settings;	/* Create the structure                          */

	tcgetattr(fd, &serial_settings);	/* Get the current attributes of the Serial port */

	cfsetispeed(&serial_settings,B19200); /* Set Read  Speed as 9600                       */
	cfsetospeed(&serial_settings,B19200); /* Set Write Speed as 9600                       */

	serial_settings.c_cflag &= ~PARENB;   /* Disables the Parity*/
	serial_settings.c_cflag &= ~CSTOPB;   /* CSTOPB = 2 Stop bits, here it is cleared so 1 Stop bit*/
	serial_settings.c_cflag &= ~CSIZE;	 /* Clears the mask for setting the data size             */
	serial_settings.c_cflag |=  CS8;      /* Set the data bits = 8                                 */

	serial_settings.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
	serial_settings.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */

	serial_settings.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
	serial_settings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Non Cannonical mode                            */

	serial_settings.c_oflag &= ~OPOST;		/*No Output Processing*/
	
	/* read at least this bytes */
	serial_settings.c_cc[VMIN] = 1;

	/* no minimum time to wait before read returns */
	serial_settings.c_cc[VTIME] = 0;

	tcflush(fd, TCIFLUSH);
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
	 
	uint8_t buf[4] = { CS_FLAG, 5, CWISE, 10 };
	tcflush(*fd, TCOFLUSH);
	if( write(*fd, buf, 4) != 4 ){
    printf("Error: bytes send should be %lu\n", sizeof(buf));
		usleep(2000*1000);
    exit(EXIT_FAILURE);
  }
	tcflush(*fd, TCIOFLUSH);

	if( close(*fd) ){
		printf("Error: close(fd) syscall failed\n");
		usleep(2000*1000);
		exit(EXIT_FAILURE);
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool handshake(int fd){
	printf("# Handshake\n");
	int ret, i = 0;
	uint8_t buf[4], byte;
	tcflush(fd, TCIFLUSH);

	//read 4 bytes(1 packet), one by one
	while(read(fd, &byte, 1)>0){
		buf[i] = byte;
		i++;
		if( i > sizeof(packet_t) ) break;
	}
	printf("    PC <---[ %d %d %d ]<--- AVR\n", buf[0],buf[1],buf[2]);
	printf("    |\n    check...\n    |\n");
	usleep(400*1000);
	if( buf[2]!=OS_FLAG ){
		return false;
	}
	tcflush(fd, TCIFLUSH);

	buf[0] = OS_FLAG;
	buf[1] = 105;
	buf[2] = CWISE;
	if( write(fd, buf, 4) != 4 ) return false;
	printf("    PC --->[ %d %d %d ]---> AVR\n", buf[0],buf[1],buf[2]);
	tcflush(fd, TCOFLUSH);
	printf("# Done.\n\n");
	usleep(400*1000);
	return true;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool writePacket(int fd, packet_t* packet){
	tcflush(fd, TCIFLUSH);
	int bytes_written = write(fd, (uint8_t*)packet, sizeof(packet_t));
	if( bytes_written == sizeof(packet_t) ){
		return true;
	}
	else {
		printf("Error: %d bytes written, but should be %d\n",
		bytes_written, (int)sizeof(packet_t));
		return false;
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool readPacket(int fd, packet_t* packet){
	tcflush(fd, TCIFLUSH);
	uint8_t buf[8], byte, i = 0;
	//read 4 bytes(1 packet), one by one
	while( read(fd, &byte, 1)>0 ){
		buf[i] = byte;
		i++;
		if( i > sizeof(packet_t) ) break;
	}
	memcpy(packet, buf, 3);
	return true;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void printPacket(packet_t packet){
	char* dir_str = "???";
	dir_str = packet.direction==CWISE ? "CW" : "CCW";
	printf("    ________________________________________________\
					\n\n      timestamp: %d   |  speed: %d  |  direction: %s  \
					\n    ________________________________________________\n\n",
          packet.timestamp, packet.speed, dir_str);
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool decreaseSpeed(packet_t* packet){
	if(packet->speed >= 105){
		packet->speed -= 5;
		return true;
	}
	else{
		packet->speed = 100;
		return false;
	}
}
bool increaseSpeed(packet_t* packet){
	if(packet->speed <= 195){
		packet->speed +=5;
		return true;
	}
	else{
		packet->speed = 200;
		return false;
	}
}

bool changeDirection(packet_t* packet){
	if( packet->direction==CWISE ) packet->direction = CCWISE;
	else packet->direction = CWISE;
	return true;
}

bool decreaseRefreshRate(packet_t* packet){
	if(packet->timestamp > 1){
		packet->timestamp--;
		return true;
	}
	else return false;
}
bool increaseRefreshRate(packet_t* packet){
	if(packet->timestamp < 5){
		packet->timestamp++;
		return true;
	}
	else return false;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/