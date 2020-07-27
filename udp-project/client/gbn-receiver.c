#include <stdio.h>		
#include <sys/socket.h>		
#include <arpa/inet.h>		
#include <stdlib.h>		
#include <string.h>		
#include <unistd.h>		
#include <errno.h>		
#include <signal.h>		

#include "defines.h"


State* state_recv;

void init_state_receiver(){
	state_recv->window = 5;
	state_recv->expected_seq_no = -1;
}


int reliable_receive_packet( void* buffer, double prob_loss, int sockfd, struct sockaddr_in * addr_ptr){

	Packet current_packet;
	int n;
	socklen_t len;

	memset(&current_packet, 0, sizeof(current_packet));
	len = sizeof(*addr_ptr);
	/* receive GBN packet */

	n = recvfrom (sockfd, &current_packet, sizeof(current_packet), 0,(struct sockaddr *) addr_ptr, &len);
	if ( n < 0 ) {
            perror ( " recvfrom error " );
            exit(1);
    }

    current_packet.length = ntohl (current_packet.length); 
    current_packet.seq_no = ntohl (current_packet.seq_no);

	/*if( prob_loss > drand48() ){
		printf("packet loss\n");
		exit(0);
	}*/
	printf (">>> Received packet %d with length %d\n", current_packet.seq_no, current_packet.length);

	/* Send ack and store in buffer */
	state_recv->expected_seq_no++;

  	printf("Expected sequence number: %d\n", state_recv->expected_seq_no );
  	if (current_packet.seq_no == state_recv->expected_seq_no )
    {
      	
      	memcpy (&buffer[PACKET_SIZE * current_packet.seq_no], current_packet.data, current_packet.length);
    }
    printf (">>> Send ack %d\n", state_recv->expected_seq_no );
    Packet current_ack; /* ack */
	current_ack.seq_no = htonl (state_recv->expected_seq_no );
	current_ack.length = htonl(0);

	if (sendto(sockfd, &current_ack, sizeof(current_ack), 0,(struct sockaddr *) addr_ptr,
		       sizeof (*addr_ptr)) !=  sizeof(current_ack) ){
		    	perror(" sent a different number of bytes ");
		    	exit(1);
	}

	return current_packet.length;  

}


int start_receiver( Datagram *datagram, int sockfd, struct sockaddr_in * addr_ptr, double prob_loss ){

		void* buffer;
		int   datasize = sizeof(*datagram);
		int   size = 0, n;
		buffer = malloc( datasize );
		state_recv = malloc(sizeof(*state_recv));

		init_state_receiver();
		printf("Start receiving bytes\n");

		while( (n = reliable_receive_packet( buffer, prob_loss, sockfd, addr_ptr) ) == PACKET_SIZE ){
			size += n;
		}
		size += n;


		printf("Bytes received %d\n", size );

		//datagram = malloc( size*sizeof(char));

		memcpy( datagram, buffer, size);

		if( (datagram->err) =! 0 ){  // there are some signal error
			return -1;
		}

		return size;



}