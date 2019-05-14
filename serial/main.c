
#include "serial.h"


int main(){

    uint8_t buf_send[4], buf_rcv[4];
    int fd, ret = -1;
	bool running = true;
	
    openSerialCommunication(&fd);
    setSerialAttributes(fd);
    
    /* testing with echo server */
    while( running ){
    	ret = write(fd, buf_send, sizeof(buf_send));
    	printf("%d bytes write to avr\n", ret);
    
    	ret = read(fd, buf_rcv, sizeof(buf_rcv));
    	printf("%d bytes read from avr\n", ret);
    
    	tcflush(fd, TCIOFLUSH);
	}

    printf("\nclosing serial communication... ");
    closeSerialCommunication(&fd);
    
    return 0;
}
