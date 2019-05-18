
#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <fcntl.h>   /* File Control Definitions           */
#include <termios.h> /* POSIX Terminal Control Definitions */
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#define BAUD_RATE 	19200
#define MYUBRR 		(F_CPU/16/BAUD_RATE-1)

#define		CWISE     0xAA
#define		CCWISE    0xBB
#define     OS_FLAG   0x3E      // > opens serial flag
#define     CS_FLAG   0x3C      // < close serial flag

#define     ttyACM0     "/dev/ttyACM0"
#define     ttyACM1     "/dev/ttyACM1"
#define     ttyACM2		"/dev/ttyACM2"
#define     ttyACM3		"/dev/ttyACM3"
#define     ttyACM4	    "/dev/ttyACM4"

extern const char* serialPorts[5];

typedef struct packet_t {
    uint8_t timestamp;
    uint8_t speed;
    uint8_t direction;
}packet_t;

typedef struct listener_params_t{
    bool*       running_ptr;
    int         fd;
    packet_t*   packet_ptr;
}listener_params_t;


//___funcion signature_________________________________

int	 openSerialCommunication(int* fd);
void setSerialAttributes(int fd);
void closeSerialCommunication(int* fd);

bool handshake(int fd);
bool writePacket(int fd, packet_t* packet);
bool readPacket(int fd, packet_t* packet);
void printPacket(packet_t packet);

bool decreaseSpeed(packet_t* packet);
bool increaseSpeed(packet_t* packet);
bool changeDirection(packet_t* packet);
bool decreaseRefreshRate(packet_t* packet);
bool increaseRefreshRate(packet_t* packet);

#endif
