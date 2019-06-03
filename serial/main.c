#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include "serial.h"
#include "gui.h"
#include <getopt.h>

sem_t mutex;
char  speed_str[64], dir_str[64],
      refresh_str[64], sendedr_win_str[256];

void flagHandler(int argc, char**argv, bool* smooth);
void* listenerRoutine(void* params);

/*===========================================================================*/
/*:::::::::::::: M A I N ::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*===========================================================================*/
int main(int argc, char* argv[]){
  bool smooth = false;

  /* flag handler */
  flagHandler(argc, argv, &smooth);

  /* struct for delay management */
  struct timespec ts, ts_rem;
  ts.tv_sec = 0;
  ts.tv_nsec = 500*1000000;
  nanosleep(&ts, &ts_rem);

  /* basic variables declaration */
  int fd = -1, ret;
  bool running = true;

  uint8_t buf_send[4] = { 1,25,CWISE,10 },
          buf_rcv[4] = { 0xFF,0xFF,0xFF,10 };
  packet_t packet_send, packet_rcv;
  memcpy(&packet_send, buf_send, 3);
  memcpy(&packet_rcv, buf_rcv, 3);

  //initialize serial communication -----------------------
  openSerialCommunication(&fd);
  setSerialAttributes(fd);
  if( smooth ) printf("\n# Linear interpolation: ON\n");

  //Handshake routine -------------------------------------
  if( !handshake(fd, &packet_rcv, smooth) ){
    printf("Handshake failed! exit... \n");
    tcflush(fd, TCOFLUSH);
    exit(EXIT_FAILURE);
  }
  //prepare listener thread parameters --------------------
  listener_params_t params = {
    .running_ptr = &running,
    .fd = fd,
    .packet_ptr = &packet_rcv
  };

  sem_init(&mutex, 0, 1);
  //create parallel thread --------------------------------
  pthread_t listener_thread;
  
  /* start critical section */
  sem_wait(&mutex);
  ret = pthread_create( &listener_thread,
                        NULL,
                        listenerRoutine,
                        (void*)&params);                        
  if( ret != 0 ){
    printf("Error: cannot create listener thread!\n");
    exit(EXIT_FAILURE);
  }
  else printf("# Listener thread successfully created.\n");
  nanosleep(&ts, &ts_rem);

  //create GUI --------------------------------------------
  WINDOW *menu_win, *sender_win;
  printf("# Initializing GUI...\n");
  nanosleep(&ts, &ts_rem);
	initGUI();
  printWelcomeMessage(welcome_msg);

  int choice, highlight = 0;
  int x_max = 1, y_max = 1;
  getmaxyx(stdscr, y_max, x_max); //(this is a macro)

  //initialize windows (size_y, size_x, pos_y, pos_x)
  menu_win = newwin(8, MENU_BOX_WIDTH, MENU_YPOS, MENU_XPOS);
  sender_win = newwin(6, 30, MENU_YPOS+1, MENU_XPOS+16);

  box(menu_win, 0, 0);
  mvwprintw(menu_win, 0, MENU_BOX_WIDTH/2-4, " SETTINGS ");
  printSender(sender_win, packet_send, speed_str, dir_str, refresh_str);
  keypad(menu_win, true);

  //refresh stuff -----------------------------------------
  refresh();
  wrefresh(menu_win);
  wrefresh(sender_win);
  clear();

  ts.tv_nsec = 33*1000000;
  bool something_change = true;
  running = true;

  /* end critical section */
  sem_post(&mutex); // --> listener start now

/*::::::::: SENDER LOOP :::::::::::::::::::::::::::::::::::::::::::::::::::::*/
  while( running ){
      
    if( something_change ) {
      if( !writePacket(fd, &packet_send) ){
        endwin();
        printf("Error: bytes send should be %lu\n", sizeof(buf_send));
        exit(EXIT_FAILURE);
      }
      something_change = false;
      nanosleep(&ts, &ts_rem);  //remove?
    }

    for(int i=0 ; i<NUM_CHOICES ; i++){
      if(i == highlight) wattron(menu_win, A_REVERSE | A_BOLD);
      mvwprintw(menu_win, i+2, 3, choices[i]);
      wattroff(menu_win, A_REVERSE | A_BOLD);
    }

    choice = wgetch(menu_win);  //blocking
    switch(choice){

      case KEY_UP:
        highlight--;
        if(highlight == -1) highlight = NUM_CHOICES-1;
        break;

      case KEY_DOWN:
        highlight++;
        if(highlight == NUM_CHOICES) highlight = 0;
        break;

      case KEY_LEFT:
        if( highlight == 0 ){
          something_change = decreaseSpeed(&packet_send);
          createSpeedString(speed_str, packet_send.speed);
        }
        if( highlight == 1 ){
          something_change = changeDirection(&packet_send);
          createDirectionString(dir_str, packet_send.direction);
        }
        if( highlight == 2 ){
          something_change = decreaseRefreshRate(&packet_send);
          createPacketRateString(refresh_str, packet_send.timestamp);
        }
        break;

        

      case KEY_RIGHT:
        if( highlight == 0 ){
          something_change = increaseSpeed(&packet_send);
          createSpeedString(speed_str, packet_send.speed);
        }
        if( highlight == 1 ){
          something_change = changeDirection(&packet_send);
          createDirectionString(dir_str, packet_send.direction);
        }
        if( highlight == 2 ){
          something_change = increaseRefreshRate(&packet_send);
          createPacketRateString(refresh_str, packet_send.timestamp);
        }
        break;

      case 10:  //CASE 'ENTER'
        if( highlight == NUM_CHOICES-1 ){
          running = false;
        }
        break;

      default:
        break;
		}

    printSender(sender_win, packet_send, speed_str, dir_str, refresh_str);
    wrefresh(sender_win);
  }/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

  //close everything --------------------------------------
  pthread_join(listener_thread, NULL);
  sem_destroy(&mutex);
  closeSerialCommunication(&fd);
  endwin();
  printf("# Serial communication closed. Exit...\n");
  nanosleep(&ts, &ts_rem);
  
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief flag handler used to manage user flags
 * 
 * @param argc from main signature
 * @param argv from main signature
 * @param smooth pointer to a boolean, enable linear interpolation on avr
 */
void flagHandler(int argc, char**argv, bool* smooth){
  *smooth = false;
  int opt;
  while((opt = getopt (argc, argv, "fld")) != -1){
    switch (opt){
      
      case 'f':
        printf("# Forcing AVR reboot... \n");
        system("cd ../avr && bash upload.sh");
        break; 

      case 'l': /* enable linear interpolation on avr */
        *smooth = true;
        break;

      case 'd':  /* deprecated */
        debug_mode();
        exit(EXIT_SUCCESS);
        break; 

      case '?':
        printf("# Avaible flags:\
          \n    -f : force avr reboot\
          \n    -l : enable avr linear interpolation\
          \n    -d : debug mode (deprecated)\n");
        exit(EXIT_FAILURE);
    } 
  }
}

/*===========================================================================*/
/*::::::::: LISTENER THREAD :::::::::::::::::::::::::::::::::::::::::::::::::*/
/*===========================================================================*/
/**
 * @brief non-blocking parallel listener
 * 
 * @param params pointer to a listener_params_t struct
 * @return void* 
 */
void* listenerRoutine(void* params){
  int ret;
  char str[4];
  listener_params_t* p = params;

  sem_wait(&mutex);
  WINDOW* listener_win = newwin(7, LISTENER_BOX_WIDTH, MENU_YPOS+10, MENU_XPOS+8);
  box(listener_win, 0, 0);
  mvwprintw(listener_win, 0, 7, " RECEIVED PACKETS ");
  mvwprintw(listener_win, 2, 3, "timestamp: ");
  mvwprintw(listener_win, 3, 3, "speed: ");
  mvwprintw(listener_win, 4, 3, "direction: ");
  wrefresh(listener_win);
  sem_post(&mutex);

  //infinite loop (untill running change) -----------------
  while(*p->running_ptr){ 
  	readPacket(p->fd, p->packet_ptr);

    createNumberString(str, p->packet_ptr->timestamp);
    mvwprintw(listener_win, 2, 3, "timestamp: %s", str);  
    createNumberString(str , p->packet_ptr->speed);
    mvwprintw(listener_win, 3, 3, "speed: %s", str);
      
    mvwprintw(listener_win, 4, 3, "direction: %s", 
      p->packet_ptr->direction==CCWISE ?
      "counterclockwise ":"clockwise       ");
		wrefresh(listener_win);
	}

  //destroy window and gui stuff
  printf("# Listener thread successfully closed.\n");
}
///////////////////////////////////////////////////////////////////////////////
