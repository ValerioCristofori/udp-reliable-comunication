#include <stdio.h>		
#include <sys/socket.h>		
#include <arpa/inet.h>		
#include <stdlib.h>		
#include <string.h>		
#include <unistd.h>		
#include <errno.h>		
#include <signal.h>	

#include "defines.h"

State* state;


void handler_alarm (int ign)	/* handler for SIGALRM */
{
  state->tries += 1;

}

void start_timer(){
		alarm(TIMEOUT);
}

int reliable_send_datagram( void* buffer, int sockfd, struct sockaddr * addr_ptr ){


		int   tmp_length;
		int num_packet_sent = 0;




		while( (state->next_seq_no < state->window + state->send_base) && (state->tries < MAXTRIES) ){
			Packet current_packet;
			// setup packet 
			current_packet.seq_no = htonl(num_packet_sent);
			if (( sizeof(buffer) - ((num_packet_sent) * state->packet_size)) >= state->packet_size) /* length packet_size doesnt except datagram */
		        tmp_length = state->packet_size;
		    else
		        tmp_length = sizeof(buffer) % state->packet_size;
		    current_packet.length = htonl(tmp_length);
		    memcpy (current_packet.data, buffer + ((num_packet_sent) * state->packet_size), tmp_length); /*copy buffer data */


		    if (sendto(sockfd, &current_packet, sizeof(current_packet), 0,(struct sockaddr *) addr_ptr,
		       sizeof (addr_ptr)) !=  (sizeof(int)*2 + tmp_length) ){
		    	perror(" sent a different number of bytes ");
		    	exit(1);
		    }


			if( state->send_base == state->next_seq_no ){
				start_timer();
			}

			state->next_seq_no++;
			num_packet_sent++;

		}

		return num_packet_sent;
}



int reliable_receive_ack( int sockfd, struct sockaddr * addr_ptr ){

	Packet ack;
	int    byte_response;
	int    len;

	len = sizeof(*addr_ptr);
	while( (byte_response = recvfrom(sockfd, &ack, sizeof (int) * 2, 0,(struct sockaddr *) addr_ptr, &len)) < 0){
			if (errno == EINTR)	/* Alarm went off  */
	  		{
	    		if (state->tries < MAXTRIES)	/* incremented by signal handler */
	      		{
					printf ("timed out, %d more tries...\n", MAXTRIES - state->tries);
					break;
	      		}
	    		else{
	    			perror("No Response");
	    			exit(0);
	    		}
	      	
	  		}
			else{
				perror("recvfrom() failed");
				exit(0);
			}

	}

	if( byte_response ){

			int ackno = ntohl(ack.seq_no);\

			state->send_base = ackno + 1;
			printf ("received ack\n");

			if( state->send_base == state->next_seq_no ){
				alarm (0); /* clear alarm */
				state->tries = 0;
			}
			else{ // not all packet acked
				alarm(TIMEOUT); /* reset alarm */
				state->tries = 0;
			}

	}



}



int start_sender( Datagram* datagram, int sockfd, struct sockaddr * addr_ptr ){

	struct sigaction act;
	void* buffer;
	int   datasize = sizeof(datagram);
	int num_packet = datasize/state->packet_size;

	buffer = malloc( datasize );
	memcpy(buffer, datagram, datasize);

	act.sa_handler = handler_alarm;
	if (sigfillset (&act.sa_mask) < 0){	/* block everything in handler */
	    perror("sigfillset() failed");
	    exit(0);
	}
	act.sa_flags = 0;

    if (sigaction (SIGALRM, &act, 0) < 0){
		perror("sigaction() failed for SIGALRM");
	    exit(0);
	}

	while( state->send_base < num_packet ){

		reliable_send_datagram( buffer, sockfd, addr_ptr );

		reliable_receive_ack( sockfd, addr_ptr );

	}

	printf("Datagram send with success\n");

}





