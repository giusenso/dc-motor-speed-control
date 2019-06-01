#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include "serial.h"
#include "gui.h"
#include <getopt.h>


sem_t mutex;

char  speed_str[64], dir_str[64],
      refresh_str[64], sendedr_win_str[256];
struct timespec ts, ts_rem;

///////////////////////////////////////////////////////////////////////////////
void* listenerRoutine(void* params){
  int ret;
  char str[4];
  listener_params_t* p = params;

  sem_wait(&mutex);
  WINDOW* listener_win = newwin(7, LISTENER_BOX_WIDTH, MENU_YPOS+10, MENU_XPOS+9);
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
      p->packet_ptr->direction==CCWISE ? "counterclockwis ":"clockwise       ");
		wrefresh(listener_win);
	}

  //destroy window and gui stuff
  printf("# Listener thread successfully closed.\n");
}
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
  ts.tv_sec = 2;
  nanosleep(&ts, &ts_rem);
  ts.tv_sec = 0;
  ts.tv_nsec = 33*1000000;

  int opt;
  while((opt = getopt (argc, argv, "d")) != -1){
    switch (opt){
      case 'd':
        debug_mode();
        exit(0);
        break;  
      /*
      case 's':
        //enable linear interpolation... 
        break;
      */
      case '?':
        exit(EXIT_FAILURE);
    } 
  }

  int choice, highlight = 0;
  int fd = -1, ret;
  bool running = true;
  sem_init(&mutex, 0, 1);

  uint8_t buf_send[4]={OS_FLAG,25,CWISE,0}, buf_rcv[4]={0xFF,0xFF,0xFF,0};

  packet_t packet_send, packet_rcv;
  memcpy(&packet_send, buf_send, 3);
  memcpy(&packet_rcv, buf_rcv, 3);

  //initialize serial communication ---------------------
  openSerialCommunication(&fd);
  setSerialAttributes(fd);

  //Handshake routine, if fail crash;
  if( !handshake(fd, &packet_rcv, &packet_send) ){
    perror("Handshake failed! exit...");
    exit(EXIT_FAILURE);
  }

  //prepare listener thread parameters ------------------
  listener_params_t params = {
    .running_ptr = &running,
    .fd = fd,
    .packet_ptr = &packet_rcv
  };

  //create parallel thread ------------------------------
  pthread_t listener_thread;
    
  sem_wait(&mutex);//start critical section
  ret = pthread_create( &listener_thread,
                        NULL,
                        listenerRoutine,
                        (void*)&params);                        
  if( ret != 0 ){
    perror("Error: cannot create listener thread!\n");
    exit(EXIT_FAILURE);
  }
  else printf("# Listener thread successfully created.\n");

  //create GUI ------------------------------------------
  WINDOW *menu_win, *sender_win;
  printf("# initializing GUI...\n");
	initGUI();
  printWelcomeMessage(welcome_msg);

  int x_max=1, y_max=1;
  getmaxyx(stdscr, y_max, x_max); //this is a macro

  //initialize windows (size_y, size_x, pos_y, pos_x)
  menu_win = newwin(8, MENU_BOX_WIDTH, MENU_YPOS, MENU_XPOS);
  sender_win = newwin(6, 30, MENU_YPOS+1, MENU_XPOS+16);

  box(menu_win, 0, 0);
  mvwprintw(menu_win, 0, MENU_BOX_WIDTH/2-4, " SETTINGS ");
  printSender(sender_win, packet_send, speed_str, dir_str, refresh_str);
  keypad(menu_win, true);

  //refresh everything ----------------------------------
  refresh();
  wrefresh(menu_win);
  wrefresh(sender_win);
  clear();

  bool something_change = true;
  running = true;
  sem_post(&mutex); //end critical section

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
  }

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

  pthread_join(listener_thread, NULL);
  sem_destroy(&mutex);
  closeSerialCommunication(&fd);
  endwin();
  printf("# Serial communication closed...\n# exit...\n");
  nanosleep(&ts, &ts_rem);
  
  return 0;
}
