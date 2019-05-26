#include <pthread.h>
#include <semaphore.h>
#include <getopt.h>
#include "serial.h"


sem_t mutex;

///////////////////////////////////////////////////////////////////////////////
void* listenerRoutine(void* params){
  printf("\n# Listener Thread successfully created.\n");
	sem_wait(&mutex);
  //...
  listener_params_t* p = params;
  sem_post(&mutex);

	//infinite loop (untill running change) -----------------
	while(*p->running_ptr){
		readPacket(p->fd, p->packet_ptr);
		printPacket(*p->packet_ptr);
	}
  printf("# Listener thread successfully closed.\n");
}
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
  usleep(2000*1000);
  
  int opt;
  while((opt = getopt (argc, argv, "d")) != -1){
    switch (opt){
      case 'd':
        debug_mode();
        break;  
      case '?':
        exit(EXIT_FAILURE);
    } 
  }
	
  int fd = -1, ret;
  bool running = true;
  sem_init(&mutex, 0, 1);

  uint8_t buf_send[4]={1,15,CWISE,0}, buf_rcv[4]={0xFF,0xFF,0xFF,0};
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
    perror("Error: cannot create Listener Thread!\n");
    exit(EXIT_FAILURE);
  }

  if( !writePacket(fd, &packet_send) ) exit(EXIT_FAILURE);
  running = true;
  sem_post(&mutex); //end critical section


/*::::::::: SENDER LOOP :::::::::::::::::::::::::::::::::::::::::::::::::::::*/
  while( running ){
    if( !writePacket(fd, &packet_send) ) exit(EXIT_FAILURE);
    usleep(1000*1000);
    if(packet_rcv.timestamp==10) running = false;
    
  }

  pthread_join(listener_thread, NULL);
  sem_destroy(&mutex);
  closeSerialCommunication(&fd);
  printf("# Closing serial communication... Done.\n");
  return 0;
}
