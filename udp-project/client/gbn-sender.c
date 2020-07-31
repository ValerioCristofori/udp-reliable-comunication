#include <stdio.h>		
#include <sys/socket.h>		
#include <arpa/inet.h>		
#include <stdlib.h>		
#include <string.h>		
#include <unistd.h>		
#include <errno.h>		
#include <signal.h>	

#include "defines.h"

State* state_send;
int ackno;
int window_ack;
int byte_reads;


void handler_alarm (int ign)	/* handler for SIGALRM */
{
  printf("-------------------------- TIMEOUT -------------------------------\n");
  state_send->next_seq_no = state_send->send_base;
  state_send->packet_sent = state_send->send_base;
  byte_reads = PACKET_SIZE;
  state_send->tries += 1;

}

void start_timer(){
		alarm(TIMEOUT);
}

void init_state_sender(){
	state_send->window = 5;
	state_send->tries = 0;
	state_send->send_base = 0;
	state_send->packet_sent = 0;
	state_send->next_seq_no = 0;
	state_send->expected_seq_no = 0;
	byte_reads = PACKET_SIZE;
	window_ack = 0;
	ackno = -1;
}

void print_state_sender(){
	printf("----STATE SENDER----\n");
	printf("Window size: %d\n", state_send->window);
	printf("Tries number: %d\n", state_send->tries);
	printf("Send base: %d\n", state_send->send_base);
	printf("Next sequence number: %d\n", state_send->next_seq_no);
	printf("Expected sequence number: %d\n", state_send->expected_seq_no);
}

int reliable_send_datagram( void* buffer, int len_buffer, int sockfd, struct sockaddr_in * addr_ptr ){


		int tmp_length, n;

		tmp_length = PACKET_SIZE;


		while( (state_send->next_seq_no < state_send->window + state_send->send_base) && (state_send->tries < MAXTRIES) && ( tmp_length == PACKET_SIZE ) ){
			
			Packet current_packet;
			// setup packet
			
			current_packet.seq_no = htonl(state_send->next_seq_no);
			printf("Trying to send packet number %d\n", state_send->next_seq_no );
			if (( len_buffer - ((state_send->packet_sent) * PACKET_SIZE)) >= PACKET_SIZE) /* length packet_size doesnt except datagram */
		        tmp_length = PACKET_SIZE;
		    else
		        tmp_length = len_buffer % PACKET_SIZE;
		    printf("Length in byte to be sent for data %d\n", tmp_length );
		    current_packet.length = htonl(tmp_length);
		    memcpy (current_packet.data, buffer + ((state_send->packet_sent) * PACKET_SIZE), tmp_length); /*copy buffer data */

		    printf("Size of the packet %ld\n", sizeof(current_packet) );
		    n = sendto(sockfd, &current_packet, (sizeof (int) * 2) + tmp_length, 0,(struct sockaddr *) addr_ptr,sizeof(*addr_ptr));
		    printf("%d\n", n );
		    if ( n !=  (sizeof(int)*2 + tmp_length) ){
		    	perror(" sent a different number of bytes ");

		    	exit(1);
		    }


			if( state_send->send_base == state_send->next_seq_no ){
				
				start_timer();
			}

			state_send->next_seq_no++;
			state_send->packet_sent++;

		}

		return tmp_length;
}



int reliable_receive_ack( int sockfd, struct sockaddr_in * addr_ptr ){

	Packet ack;
	int    byte_response;
	socklen_t    len;

	len = sizeof(*addr_ptr);
	while( (byte_response = recvfrom(sockfd, &ack, sizeof (int) * 2, 0,(struct sockaddr *) addr_ptr, &len)) < 0){
			

			if (errno == EINTR)	/* Alarm went off  */
	  		{
	    		if (state_send->tries < MAXTRIES)	/* incremented by signal handler */
	      		{
					printf ("timed out, %d more tries...\n", MAXTRIES - state_send->tries);
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


	ackno = ntohl(ack.seq_no);
	printf("Window of acks %d\n", window_ack );


	if( byte_response && ( (window_ack == ackno + 1) || (window_ack == ackno - 1) || (window_ack == ackno) ) ){


			window_ack = ackno + 1;
			state_send->send_base = (ackno + 1);
			printf ("received ack %d\n", ackno);

			if( state_send->send_base == state_send->next_seq_no ){
				window_ack--;
				alarm (0); /* clear alarm */
				state_send->tries = 0;
				return ackno;

			}
			else{ // not all packet acked
				printf("Restart TO\n");
				alarm(TIMEOUT); /* reset alarm */
				state_send->tries = 0;
			}

	}

	

	return byte_response;

}



void start_sender( Datagram* datagram, int size, int sockfd, struct sockaddr_in * addr_ptr ){

	struct sigaction act;
	char *buffer;
	printf("Datagram dimension %d\n", size );
	int num_packet = (size/PACKET_SIZE) + 1;
	printf("%d\n", num_packet );
	printf("Number of packet to be send %d\n", num_packet);



	printf("Build buffer of bytes\n");
	//build the buffer in a separate function
	buffer = malloc(size);
	memcpy(buffer, datagram, size);


	printf("buffer %s\n", (char *)buffer );

	state_send = malloc(sizeof(*state_send));
	act.sa_handler = handler_alarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

    if (sigaction (SIGALRM, &act, NULL) < 0){
		perror("sigaction() failed for SIGALRM");
	    exit(0);
	}


	init_state_sender();
	print_state_sender();
	do{
		if(byte_reads >= PACKET_SIZE)
			byte_reads = reliable_send_datagram( buffer, size, sockfd, addr_ptr );

		reliable_receive_ack( sockfd, addr_ptr );

		print_state_sender();
		printf("------ packet sent %d\n", state_send->packet_sent );
	}
	while( ackno != num_packet - 1 ); 

	printf("Datagram sent with success\n");


}





