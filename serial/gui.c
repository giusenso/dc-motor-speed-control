#include <ncurses.h>
#include "gui.h"


char* choices[NUM_CHOICES] = {	" Speed ",
								" Direction ",
								" Packet rate ",
								" exit " 
							 };
							 
char* welcome_msg = "~ DC MOTOR CONTROL ~";
int choice, highlight = 0;

void initGUI(){

	initscr();		//create stdscr
	noecho();
	cbreak();		// enable ^C escape 
	curs_set(0);	//invisible cursor

	if(!has_colors()){
      printw("Terminal does not support colors");
      //return -1;
    }
}

void createSpeedString(char* str, uint8_t speed){
	int len = 20;
	char str_speed[4];
	snprintf(str_speed, sizeof(str_speed), "%d", speed);
	const uint8_t max_speed_len = speed/5;
	char speed_level[len];

	memset(str, 0, sizeof(str));
	strcat(str, "[");
	for( int i=0 ; i<len ; i++ ){
		if(i<max_speed_len) speed_level[i]='|';
		else speed_level[i]=' ';
	}
	strcat(str, speed_level);
	if(speed<10) strcat(str, "   ");
	else if(speed<100) strcat(str, "  ");
	else strcat(str, " ");
	strcat(str, str_speed);
	strcat(str, "% ]");
}

char* createNumberString(char* str, uint8_t x){
	if( x<10 ) snprintf(str, sizeof(str),"%d   ",x);
	else if( x<100 ) snprintf(str,sizeof(str),"%d  ",x);
	else snprintf(str, sizeof(str),"%d ",x);
	return str;
}

void createDirectionString(char* str, uint8_t direction){
	memset(str, 0, sizeof(str));
	strcat(str, "[");
	if(direction==CWISE){
		strcat(str, "         ");
		strcat(str, "clockwise");
		strcat(str, "        ");
	}
	else if(direction==CCWISE){
		strcat(str, "     ");
		strcat(str, "counterclockwise");
		strcat(str, "     ");
	}
	else{
		strcat(str, " ??? ");
	}
	strcat(str, "]\n");
}

void createPacketRateString(char* str, uint8_t refresh_rate){
	memset(str, 0, sizeof(str));
	char str_refresh_rate[4];
	snprintf(str_refresh_rate, sizeof(str_refresh_rate), "%d", refresh_rate);
	strcat(str, "[      ");
	strcat(str, str_refresh_rate);
	strcat(str, " every second");
	strcat(str, "      ]");
}

void concatStrings(char* output, char* str1, char* str2, char* str3){
	memset(output, 0, sizeof(output));
	strcat(output, str1);
	strcat(output, str2);
	strcat(output, str3);
}

void printWelcomeMessage(char* msg){
	int x_max=1, y_max=1;
	getmaxyx(stdscr, y_max, x_max);

    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);

    attron(COLOR_PAIR(1) | A_REVERSE | A_BOLD);
    mvprintw(2, MENU_BOX_WIDTH/2-4, msg);
    attroff(A_REVERSE | A_BOLD);
}


void printSender(WINDOW* sender_win, packet_t packet,
                char* speed_str, char* dir_str, char* refresh_str ){
	
	createSpeedString(speed_str, packet.speed);
	createDirectionString(dir_str, packet.direction);
	createPacketRateString(refresh_str, packet.timestamp);
    mvwprintw(sender_win, 1, 1, "%s", speed_str);
	mvwprintw(sender_win, 2, 1, "%s", dir_str);
	mvwprintw(sender_win, 3, 1, "%s", refresh_str);		
				
}
