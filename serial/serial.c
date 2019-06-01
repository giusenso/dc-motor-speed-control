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
	 
	packet_t* tmp = (packet_t*)malloc(sizeof(packet_t));
	tmp->timestamp = CS_FLAG;
	tmp->speed = CS_FLAG;
	tmp->direction = CS_FLAG;
	if( !writePacket(*fd, tmp) ) exit(EXIT_FAILURE);
	free(tmp);

	if( close(*fd) ){
		printf("Error: close(fd) syscall failed\n");
		usleep(2000*1000);
		exit(EXIT_FAILURE);
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool handshake(int fd, packet_t* packet_rcv, packet_t* packet_send){
	struct timespec ts = {0,300*1000000}, ts_rem;
	printf("# Handshake\n====================================\n\n");
	
	packet_t open_packet = { OS_FLAG, OS_FLAG, OS_FLAG };
	if( !readPacket(fd, packet_rcv) ) return false;
	printf("    PC <<<<< ");
	printPacketV2(*packet_rcv);
	printf(" <<<<< AVR\n    |\n    check... ");
	if ( !packetcmp(packet_rcv, &open_packet) ) {
		printf("error! packet not match with open_packet\n");
		return false;
	}
	else printf(" no errors.\n    |\n");
	nanosleep(&ts, &ts_rem);

	//------------------------------------------------
	if( !writePacket(fd,&open_packet) ) return false;
	printf("    PC >>>>> ");
	printPacketV2(open_packet);
	printf(" >>>>> AVR\n");

	if( !readPacket(fd, packet_rcv) ) return false;
	printf("    PC <<<<< ");
	printPacketV2(*packet_rcv);
	printf(" <<<<< AVR\n    |\n    check... ");
	if ( !packetcmp(packet_rcv, &open_packet) ) {
		printf("error! packet not match with open_packet\n");
		return false;
	}
	else printf(" no errors.\n    |\n");
	nanosleep(&ts, &ts_rem);

	//------------------------------------------------
	//write starting configuration
	packet_send->timestamp = OS_FLAG;

	if( !writePacket(fd, packet_send) ) return false;
	printf("    PC >>>>> ");
	printPacketV2(*packet_send);
	printf(" >>>>> AVR\n");

	if( !readPacket(fd, packet_rcv) ) return false;
	printf("    PC <<<<< ");
	printPacketV2(*packet_rcv);
	printf(" <<<<< AVR\n    |\n    check... ");
	if ( !packetcmp(packet_rcv, packet_send) ) {
		printf("error! packet not match with packet_send.\n");
		return false;
	}
	else printf(" no errors.\n    |\n");
	packet_send->timestamp = 1;

	//------------------------------------------------
	printf("\n====================================\n");
	nanosleep(&ts, &ts_rem);
	tcflush(fd, TCIOFLUSH);
	return true;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool writePacket(int fd, packet_t* packet){
	tcflush(fd, TCOFLUSH);
	uint8_t buf[4] = { 0,0,0,10 };
	memcpy(buf, packet, 3);
	buf[1] += 100;

	if( write(fd, buf, sizeof(buf)) == sizeof(buf) ){
		//printf("write: [ %d %d %d %d ]\n", buf[0],buf[1],buf[2],buf[3]);
		return true;
	}
	else {
		printf("Error: bytes written should be %lu\n",sizeof(buf));
		return false;
	}
	
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool readPacket(int fd, packet_t* packet){
	tcflush(fd, TCIFLUSH);
	uint8_t buf[4], byte, i = 0;

	while( read(fd, &byte, 1)>0 ){
		buf[i] = byte;
		i++;
		if( i > sizeof(packet_t) ) break;
	}
	//printf("read: [ %d %d %d %d ]\n", buf[0],buf[1],buf[2],buf[3]);
	memcpy(packet, buf, 3);
	packet->speed -= 100;
	return true;

}

bool packetcmp(packet_t* p1, packet_t* p2){
	if( p1->timestamp != p2->timestamp ) return false;
	if( p1->speed != p2->speed ) 		 return false;
	if( p1->direction != p2->direction ) return false;
	else return true;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void printPacket(packet_t packet){
	char* str_dir = "???";
	if( packet.direction==CWISE ) str_dir = "CW"; 
	else if( packet.direction==CCWISE ) str_dir = "CCW";
	printf("\n  [ ts: %d | speed: %d%% | dir: %s ]\n",
          packet.timestamp, packet.speed, str_dir);
}

void printPacketV2(packet_t packet){
	printf("[ %d %d %d ]", packet.timestamp, packet.speed, packet.direction);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool decreaseSpeed(packet_t* packet){
	if(packet->speed >= 5){
		packet->speed -= 5;
		return true;
	}
	else{
		packet->speed = 0;
		return false;
	}
}

bool increaseSpeed(packet_t* packet){
	if(packet->speed <= 95){
		packet->speed +=5;
		return true;
	}
	else{
		packet->speed = 100;
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

/**
 * @brief used to test the serial communication.
 * - open serial communication
 * - handshake
 * - set dc motor parameters
 * - read every 1 second the stats
 * 
 * @return true if there is no error
 * @return false in case of error
 */
bool debug_mode(){
	printf("\n====================================\
		\n        ### DEBUG MODE ###\
		\n====================================\n");

	int fd;
  	uint8_t buf_send[4]={OS_FLAG,15,CWISE,0},
	  		buf_rcv[4]={0xFF,0xFF,0xFF,0};

	if( openSerialCommunication(&fd) < 0 ){
		perror("Failed to open serial communication\n");
		return false;
	}	
  	setSerialAttributes(fd);

	packet_t packet_send, packet_rcv;
	memcpy(&packet_send, buf_send, 3);
 	memcpy(&packet_rcv, buf_rcv, 3);

	if( !handshake(fd, &packet_rcv, &packet_send) ){
		printf("Handshake failed!\n");
		return false;
	}

	writePacket(fd, &packet_send);
	tcflush(fd, TCIOFLUSH);
	for(int i=0 ; i<10 ; i++ ){
		readPacket(fd, &packet_rcv);
		printPacket(packet_rcv);
	}

	closeSerialCommunication(&fd);
	printf("\nNo errors occured. Great job!\
		\n====================================\n\n");
	return true;
}