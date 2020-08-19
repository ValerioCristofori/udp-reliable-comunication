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

int   				num_conn = 0;
pthread_mutex_t 	lock;




//
void init_state_sender(State *s){
	/*
	 *  Init the struct State connection
	 *  for the specific couple thread child - client
	 */

	s->window 			= 5;  //max packet in fly
	s->tries 			= 0;  //init count tries
	s->send_base 		= 0;  
	s->packet_sent 		= 0;
	s->next_seq_no 		= 0;
	s->expected_seq_no 	= 0;
	s->byte_reads 		= PACKET_SIZE;  //init to PACKET SIZE for start reading 
	s->window_ack 		= 0;
	s->ack_no 			= -1;  //init ack to -1 for the initial critical case

}


void *start_timer_thread( void *whoami ){

	/*
	 * Thread for send alarm to the thread that \
	 * manage the connection with the client
	 * 
	 * Implementative choice --> with an alarm all threads hadle it
	 */

	pthread_t *t = (pthread_t *)whoami;

	sleep(TIMEOUT);              //after TIMEOUT secs send the alarm to the specific thread
	pthread_kill( *t , SIGALRM);

	pthread_exit((void *)0);     //exit from the alarm thread
	
	

}

void clear_thread( pthread_t *th ){

	/*
	 * This procedure is used to
	 * clear all the alarms killing all the thread alive
	 */ 

	printf("Clearing all alarms\n");
	

	printf("kill %ld\n", *th );
	pthread_cancel(*th);

	
}

void handler_alarm (int ign)	/* handler for SIGALRM */
{

	/*
	 *  When the alarm expires 
	 *  the thread child restart the sending going back to n
	 *  where n is the send base ( oldest packet sent not acked yet )
	 */

			State      *s;
			pthread_t   t;
  	
			printf("-------------------------- TIMEOUT -------------------------------\n");
			
			//search for the current relation
			t = pthread_self();

			for(int i=0; i<MAX_THREADS; i++){
				if( relations[i]->th == t ){
					s = relations[i]->state;  //take the correct state of that relation ( thread-client )
					break;
				} 
			}

			s->next_seq_no = s->send_base;  //going back to n according to the protocol
			s->packet_sent = s->send_base;
			s->byte_reads = PACKET_SIZE;
			s->tries += 1;  				// upgrade the counter with one more try

			

}


int reliable_send_datagram( State *s, void* buffer, int len_buffer, int sockfd, struct sockaddr_in * addr_ptr, pthread_t whoami, pthread_t *th ){

	/*
	 *  Return the number of bytes sent to the sockaddr
	 *  The buffer is divided in some packet of PACKETSIZE length 
	 *   and is implemented the go back n protocol 
	 *   for reliable data transfer
	 */


		int tmp_length, n;

		tmp_length = PACKET_SIZE; //init for exiting in the first while loop


		while( (s->next_seq_no < s->window + s->send_base) && (s->tries < MAXTRIES) && ( tmp_length == PACKET_SIZE ) ){
			
			Packet current_packet;  //declare packet to be sent
			// setup packet
			
			current_packet.seq_no = htonl(s->next_seq_no);
			printf("Trying to send packet number %d\n", s->next_seq_no );
			if (( len_buffer - ((s->packet_sent) * PACKET_SIZE)) >= PACKET_SIZE) //length packet doesnt except packet size
		        tmp_length = PACKET_SIZE; //length is the max size
		    else
		        tmp_length = len_buffer % PACKET_SIZE;  // length is the last bytes not sent yet
		    printf("Length in byte to be sent for data %d\n", tmp_length );
		    current_packet.length = htonl(tmp_length);  //setting the length
		    memcpy (current_packet.data, buffer + ((s->packet_sent) * PACKET_SIZE), tmp_length); //copy buffer data

		    printf("Size of the packet %ld\n", sizeof(current_packet) );
		    n = sendto(sockfd, &current_packet, (sizeof (int) * 2) + tmp_length, 0,(struct sockaddr *) addr_ptr,sizeof(*addr_ptr));
		    if ( n !=  (sizeof(int)*2 + tmp_length) ){  //Sending packet with sendto func
		    	perror(" sent a different number of bytes ");

		    	exit(1);
		    }

		    if( s->send_base == s->next_seq_no ){  // when start the timer according to gbn
				
				if( *th != 0 ){   //clear all alarm threads
					printf("kill %ld\n", *th );
					pthread_cancel(*th);
				}
				pthread_create(th, NULL, start_timer_thread, (void*)whoami ); //create the timer
				printf("Created timer\n");
			}

			//update variables

			s->next_seq_no++;
			s->packet_sent++;

		}

		return tmp_length;
}



int reliable_receive_ack( State *s, int sockfd, struct sockaddr_in * addr_ptr, pthread_t whoami, pthread_t *th ){

	/*
	 *  Return the number of the byte read from the socket in sockaddr
	 *  This is implemented by the mechanism of acks
	 */


	Packet 		ack;
	int    		byte_response;
	socklen_t   len;

	ack.seq_no = -1*(s->window);

	len = sizeof(*addr_ptr);
	while( (byte_response = recvfrom(sockfd, &ack, sizeof (int) * 2, 0,(struct sockaddr *) addr_ptr, &len)) < 0){
			

			if (errno == EINTR)	// alarm went off 
	  		{
	    		if (s->tries < MAXTRIES)	// incremented by alarm handler
	      		{
					printf ("Timed out\nThere are %d more tries.\n", MAXTRIES - s->tries);
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

			if( s->send_base == s->next_seq_no ){ //received the correct ack
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

	/*
	 *  Procedure to start the sending packets to sockaddr_in
	 *  Implemented the go back n protocol for reliable data transfer
	 *  Used the probability p to simulate the data loss and
	 *  check the reliability.
	 *  
	 *  Used to send datagram struct that is copied to a buffer of bytes
	 *  And segmented in packet of PACKETSIZE length
	 *  
	 */

	struct sigaction 			act;
	char 					   *buffer;
	State 					   *s;
	pthread_t 				   *th;
	int 						num_packet = (size/PACKET_SIZE) + 1;
	
	printf("Datagram dimension %d\n", size );
	
	printf("%d\n", num_packet );
	printf("Number of packet to be send %d\n", num_packet);


	//start new sender
	pthread_mutex_lock(&lock);
	num_conn++;

	//malloc the struct relation for new sending
	relations[num_conn - 1] = malloc(sizeof(R));
	relations[num_conn - 1]->state = malloc(sizeof(State));
	s = relations[num_conn - 1]->state;
	relations[num_conn - 1]->th = whoami;

	pthread_mutex_unlock(&lock);  //release resource

	printf("Build buffer of bytes\n");
	buffer = malloc(size);
	memcpy(buffer, datagram, size);

	th = malloc(sizeof(pthread_t));


	printf("buffer %s\n", (char *)buffer );

	act.sa_handler = handler_alarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

    if (sigaction (SIGALRM, &act, NULL) < 0){  //add the handler for manage the TO
		perror("sigaction() failed for SIGALRM");
	    exit(0);
	}

	// init variables for the sending
	init_state_sender(s);
	print_state_sender(s);
	

	do{
		if( s->byte_reads >= PACKET_SIZE )
			//try to send packets in pipelined
			s->byte_reads = reliable_send_datagram( s, buffer, size, sockfd, addr_ptr, whoami, th );
		
		//wait ack
		reliable_receive_ack( s, sockfd, addr_ptr, whoami, th );

		print_state_sender(s);
		printf("------ packet sent %d\n", s->packet_sent );
	}
	while( s->ack_no != num_packet - 1 ); // while the ack is not the last one

	clear_thread(th);

	printf("Datagram sent with success\n");

	num_conn--;  //all datagram sent

}

