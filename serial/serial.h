
#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define BAUD_RATE 19200
#define MYUBRR (F_CPU/16/BAUD_RATE-1)

#define 	CWISE  		0xAA
#define 	CCWISE  	0xBB
#define 	OF  		'>'
#define 	CF  		'<'
#define 	MIN_SPEED  	100
#define 	MAX_SPEED  	200

extern const char* serialPorts[5];
#define     ttyACM0     "/dev/ttyACM0"
#define     ttyACM1     "/dev/ttyACM1"
#define     ttyACM2		"/dev/ttyACM2"
#define     ttyACM3		"/dev/ttyACM3"
#define     ttyACM4	    "/dev/ttyACM4"

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

//___function signature_________________________________

bool debug_mode();

//serial setup
int	 openSerialCommunication(int* fd);
void closeSerialCommunication(int* fd);
void setSerialAttributes(int fd);

//communication routines
bool handshake(int fd, packet_t* packet, bool smooth);
bool writePacket(int fd, packet_t* packet);
bool readPacket(int fd, packet_t* packet);
void printPacket(packet_t packet);
void printPacketV2(packet_t packet);
bool packetcmp(packet_t* p1, packet_t* p2);


//gui support functions
bool decreaseSpeed(packet_t* packet);
bool increaseSpeed(packet_t* packet);
bool changeDirection(packet_t* packet);
bool decreaseRefreshRate(packet_t* packet);
bool increaseRefreshRate(packet_t* packet);


//parallel listener
//void* listenerRoutine(void* params);

#endif