#ifndef GUI_H
#define GUI_H

#include <ncurses.h>
#include <stdint.h>
#include <string.h>
#include "serial.h"

#define		NUM_CHOICES			4
#define		MENU_XPOS			5
#define		MENU_YPOS		    4
#define     MENU_BOX_WIDTH      50
#define     LISTENER_BOX_WIDTH  32

extern char* choices[NUM_CHOICES];
extern char* welcome_msg;

void initGUI();
void printWelcomeMessage(char* msg);
char* createNumberString(char* str, uint8_t x);
void createSpeedString(char* str, uint8_t speed);
void createDirectionString(char* str, uint8_t direction);
void createPacketRateString(char* str, uint8_t refresh_rate);
void concatStrings(char* output, char* str1, char* str2, char* str3);
void handShaking(int fd);
void printSender(  WINDOW* sender_win,
                    packet_t packet,
                    char* speed_str, char* dir_str, char* refresh_str
                );

#endif