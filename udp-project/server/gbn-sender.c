#include <stdio.h>		
#include <sys/socket.h>		
#include <arpa/inet.h>		
#include <stdlib.h>		
#include <string.h>		
#include <unistd.h>		
#include <errno.h>		
#include <pthread.h>
#include <signal.h>	

#include "defines.h"

int   num_conn = 0;
pthread_mutex_t lock;





void init_state_sender(State *s){
	s->window = 5;
	s->tries = 0;
	s->send_base = 0;
	s->packet_sent = 0;
	s->next_seq_no = 0;
	s->expected_seq_no = 0;
	s->byte_reads = PACKET_SIZE;
	s->window_ack = 0;
	s->ack_no = -1;

}

void print_state_sender(State *s){
	printf("----STATE SENDER----\n");
	printf("Window size: %d\n", s->window);
	printf("Tries number: %d\n", s->tries);
	printf("Send base: %d\n", s->send_base);
	printf("Next sequence number: %d\n", s->next_seq_no);
	printf("Bytes reads: %d\n", s->byte_reads );
	printf("Expected sequence number: %d\n", s->expected_seq_no);
}

void *start_timer_thread( void *whoami ){

	pthread_t *t = (pthread_t *)whoami;

	sleep(TIMEOUT);
	pthread_kill( *t , SIGALRM);

	pthread_exit((void *)0);
	
	

}

void clear_thread( pthread_t *th ){

	printf("Clearing all alarms\n");
	

	printf("kill %ld\n", *th );
	pthread_cancel(*th);

	
}

void handler_alarm (int ign)	/* handler for SIGALRM */
{
			State      *s;
			pthread_t   t;
  	
			printf("-------------------------- TIMEOUT -------------------------------\n");
			
			//search for the current relation
			t = pthread_self();

			for(int i=0; i<MAX_THREADS; i++){
				if( relations[i]->th == t ){
					printf("Take it! on pos %d\n", i);
					s = relations[i]->state;
					break;
				} 
			}

			s->next_seq_no = s->send_base;
			s->packet_sent = s->send_base;
			s->byte_reads = PACKET_SIZE;
			s->tries += 1;

			

}


int reliable_send_datagram( State *s, void* buffer, int len_buffer, int sockfd, struct sockaddr_in * addr_ptr, pthread_t whoami, pthread_t *th ){


		int tmp_length, n;

		tmp_length = PACKET_SIZE;


		while( (s->next_seq_no < s->window + s->send_base) && (s->tries < MAXTRIES) && ( tmp_length == PACKET_SIZE ) ){
			
			Packet current_packet;
			// setup packet
			
			current_packet.seq_no = htonl(s->next_seq_no);
			printf("Trying to send packet number %d\n", s->next_seq_no );
			if (( len_buffer - ((s->packet_sent) * PACKET_SIZE)) >= PACKET_SIZE) /* length packet_size doesnt except datagram */
		        tmp_length = PACKET_SIZE;
		    else
		        tmp_length = len_buffer % PACKET_SIZE;
		    printf("Length in byte to be sent for data %d\n", tmp_length );
		    current_packet.length = htonl(tmp_length);
		    memcpy (current_packet.data, buffer + ((s->packet_sent) * PACKET_SIZE), tmp_length); /*copy buffer data */

		    printf("Size of the packet %ld\n", sizeof(current_packet) );
		    n = sendto(sockfd, &current_packet, (sizeof (int) * 2) + tmp_length, 0,(struct sockaddr *) addr_ptr,sizeof(*addr_ptr));
		    printf("%d\n", n );
		    if ( n !=  (sizeof(int)*2 + tmp_length) ){
		    	perror(" sent a different number of bytes ");

		    	exit(1);
		    }

		    if( s->send_base == s->next_seq_no ){
				
				if( *th != 0 ){
					printf("kill %ld\n", *th );
					pthread_cancel(*th);
				}
				pthread_create(th, NULL, start_timer_thread, (void*)whoami );
				printf("Created timer\n");
			}

			s->next_seq_no++;
			s->packet_sent++;

		}

		return tmp_length;
}



int reliable_receive_ack( State *s, int sockfd, struct sockaddr_in * addr_ptr, pthread_t whoami, pthread_t *th ){

	Packet ack;
	int    byte_response;
	socklen_t    len;

	ack.seq_no = -1*(s->window);

	len = sizeof(*addr_ptr);
	while( (byte_response = recvfrom(sockfd, &ack, sizeof (int) * 2, 0,(struct sockaddr *) addr_ptr, &len)) < 0){
			

			if (errno == EINTR)	/* Alarm went off  */
	  		{
	    		if (s->tries < MAXTRIES)	/* incremented by signal handler */
	      		{
					printf ("timed out, %d more tries...\n", MAXTRIES - s->tries);
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


	s->ack_no = ntohl(ack.seq_no);
	printf("Window of acks %d\n", s->window_ack );


	if( byte_response && ( (s->window_ack == s->ack_no + 1) || (s->window_ack == s->ack_no - 1) || (s->window_ack == s->ack_no) ) ){


			s->window_ack = s->ack_no + 1;
			s->send_base = (s->ack_no + 1);
			printf ("received ack %d\n", s->ack_no);

			if( s->send_base == s->next_seq_no ){
				s->window_ack--;
				clear_thread(th);
				s->tries = 0;
				return s->ack_no;

			}
			else{ // not all packet acked
				printf("Restart TO\n");
				if( *th != 0 ){
					printf("kill %ld\n", *th );
					pthread_cancel(*th);
				}
				pthread_create(th, NULL, start_timer_thread, (void*)whoami );
				printf("Created timer\n");
				
			}

	}

	

	return byte_response;

}



void start_sender( Datagram* datagram, int size, int sockfd, struct sockaddr_in * addr_ptr, pthread_t whoami ){

	struct sigaction act;
	char *buffer;
	State *s;
	pthread_t *th;
	printf("Datagram dimension %d\n", size );
	int num_packet = (size/PACKET_SIZE) + 1;
	printf("%d\n", num_packet );
	printf("Number of packet to be send %d\n", num_packet);


	//start new sender
	pthread_mutex_lock(&lock);
	num_conn++;

	relations[num_conn - 1] = malloc(sizeof(R));
	relations[num_conn - 1]->state = malloc(sizeof(State));
	s = relations[num_conn - 1]->state;
	relations[num_conn - 1]->th = whoami;

	pthread_mutex_unlock(&lock);

	printf("Build buffer of bytes\n");
	//build the buffer in a separate function
	buffer = malloc(size);
	memcpy(buffer, datagram, size);

	th = malloc(sizeof(pthread_t));


	printf("buffer %s\n", (char *)buffer );

	act.sa_handler = handler_alarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

    if (sigaction (SIGALRM, &act, NULL) < 0){
		perror("sigaction() failed for SIGALRM");
	    exit(0);
	}


	init_state_sender(s);
	print_state_sender(s);
	

	do{
		if( s->byte_reads >= PACKET_SIZE )
			s->byte_reads = reliable_send_datagram( s, buffer, size, sockfd, addr_ptr, whoami, th );
		

		reliable_receive_ack( s, sockfd, addr_ptr, whoami, th );

		print_state_sender(s);
		printf("------ packet sent %d\n", s->packet_sent );
	}
	while( s->ack_no != num_packet - 1 ); 

	clear_thread(th);

	printf("Datagram sent with success\n");

	num_conn--;

}

