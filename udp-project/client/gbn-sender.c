#include "defines.h"

State  	*state_send;
int 	 ackno;
int 	 window_ack;
int 	 byte_reads;
double 	 current_timer;

double 					    dev_RTT; 
double 						estimate_RTT;
struct timeval 				time_begin, time_end;
double 						sample_RTT;



void handler_alarm (int ign)	/* handler for SIGALRM */
{
	/*
	 *  When the alarm expires 
	 *  the client restart the sending going back to n
	 *  where n is the send base ( oldest packet sent not acked yet )
	 */

			bold_red();
		  printf("-------------------------- TIMEOUT -------------------------------\n");
		  reset_color();
		  state_send->next_seq_no = state_send->send_base;  //going back to n according to the protocol
		  state_send->packet_sent = state_send->send_base;
		  byte_reads = PACKET_SIZE;
		  state_send->tries += 1;   //upgrade tries counter for interrupt critical cases

}

void init_state_sender(){
	/*
	 *  Init the struct State connection
	 *  for the specific couple thread child - client
	 */
	state_send->tries 			= 0;  //init count tries
	state_send->send_base 		= 0;
	state_send->packet_sent 	= 0;
	state_send->next_seq_no 	= 0;
	state_send->expected_seq_no = 0;
	byte_reads 					= PACKET_SIZE;  //init to PACKET SIZE for start reading 
	window_ack 					= 0;
	ackno 						= -1;  //init ack to -1 for the initial critical case
}

int reliable_send_datagram( void* buffer, int len_buffer, int sockfd, struct sockaddr_in * addr_ptr ){


	/*
	 *  Return the number of bytes sent to the sockaddr
	 *  The buffer is divided in some packet of PACKETSIZE length 
	 *   and is implemented the go back n protocol 
	 *   for reliable data transfer
	 */

		int 	tmp_length, n;

		tmp_length = PACKET_SIZE;

		while( (state_send->next_seq_no < window + state_send->send_base) && (state_send->tries < MAXTRIES) && ( tmp_length == PACKET_SIZE ) ){
			
			Packet current_packet;
			// setup packet
			
			current_packet.seq_no = htonl(state_send->next_seq_no);
			green();
			printf(">>> Trying to send packet number %d\n", state_send->next_seq_no );
			if (( len_buffer - ((state_send->packet_sent) * PACKET_SIZE)) >= PACKET_SIZE) /* length packet_size doesnt except datagram */
		        tmp_length = PACKET_SIZE;
		    else
		        tmp_length = len_buffer % PACKET_SIZE;
		    printf("Length in byte to be sent for data %d\n", tmp_length );
		    current_packet.length = htonl(tmp_length);
		    memcpy (current_packet.data, buffer + ((state_send->packet_sent) * PACKET_SIZE), tmp_length); /*copy buffer data */

		    //start timer for adaptive timer
		    if (adaptive) gettimeofday(&time_begin, NULL);

		    printf("Size of the packet %ld\n", sizeof(current_packet) );
		    n = sendto(sockfd, &current_packet, (sizeof (int) * 2) + tmp_length, 0,(struct sockaddr *) addr_ptr,sizeof(*addr_ptr));
		    printf("%d\n", n );
		    if ( n !=  (sizeof(int)*2 + tmp_length) ){
		    	perror(" sent a different number of bytes ");

		    	exit(1);
		    }
		    reset_color();

			if( state_send->send_base == state_send->next_seq_no ){  // when start the timer according to gbn
				
				alarm(current_timer);
				cyan();
				printf(">>> Created timer\n");
				reset_color();
			}

			//update variables
			state_send->next_seq_no++;
			state_send->packet_sent++;

		}

		return tmp_length;
}



int reliable_receive_ack( int sockfd, struct sockaddr_in * addr_ptr){

	/*
	 *  Return the number of the byte read from the socket in sockaddr
	 *  This is implemented by the mechanism of acks
	 */

	Packet 			ack;	
	int    			byte_response;
	socklen_t    	len;

	// init -1 for manage case: lost first packet
	ack.seq_no = -1*window;

	len = sizeof(*addr_ptr);
	while( (byte_response = recvfrom(sockfd, &ack, sizeof (int) * 2, 0,(struct sockaddr *) addr_ptr, &len)) < 0){
			

			if (errno == EINTR)	// alarm went off 
	  		{
	  			if(adaptive){
	  				current_timer = reset_adaptive_timer(&estimate_RTT, &sample_RTT, &dev_RTT);
	  			}
	    		if (state_send->tries < MAXTRIES)	// incremented by alarm handler
	      		{
	      			bold_red();
					printf ("Timed out, %d more tries\n", MAXTRIES - state_send->tries);
					reset_color();
					break;
	      		}
	    		else{
	    			red();
	    			perror("No Response");
	    			reset_color();
	    			exit(0);
	    		}
	      	
	  		}
			else{
				red();
				perror("recvfrom() failed");
				reset_color();
				exit(0);
			}

	}


	ackno = ntohl(ack.seq_no);


	if( byte_response && ( (window_ack == ackno + 1) || (window_ack == ackno - 1) || (window_ack == ackno) ) ){


			window_ack = ackno + 1;
			state_send->send_base = (ackno + 1);
			green();
			printf (">>> Received ack %d\n", ackno);
			reset_color();

			if( state_send->send_base == state_send->next_seq_no  ){  //received the correct ack
				window_ack--;
				alarm (0); /* clear alarm */
				state_send->tries = 0;
				if(adaptive){
					gettimeofday(&time_end, NULL);
					current_timer = change_adaptive_timer(time_begin, time_end, &estimate_RTT, &sample_RTT, &dev_RTT);
				}
				return ackno;

			}
			else{ // not all packet acked
				printf("Restart TO\n");
				state_send->tries = 0;
				if(adaptive){
					gettimeofday(&time_end, NULL);
					current_timer = change_adaptive_timer(time_begin, time_end, &estimate_RTT, &sample_RTT, &dev_RTT);
				}
				alarm(current_timer); /* reset alarm */
				
			}

	}

	

	return byte_response;

}



void start_sender( Datagram* datagram, int size, int sockfd, struct sockaddr_in * addr_ptr ){

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
	int 						num_packet = (size/PACKET_SIZE) + 1;

	if(adaptive){
			//init variables
			current_timer 	= timeout;
			estimate_RTT 	= timeout;	
			sample_RTT 		= 0;
			dev_RTT 		= 0;
	}
	
	printf("Number of packet to be send %d\n", num_packet);

	printf("Build buffer of bytes\n");

	//malloc and clear the buffer of bytes for sending
	buffer = malloc(size);
	memcpy(buffer, datagram, size);
	//malloc the state of the sending
	state_send = malloc(sizeof(*state_send));
	

	act.sa_handler = handler_alarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

    if (sigaction (SIGALRM, &act, NULL) < 0){ //add the handler for manage the TO
    	red();
		perror("sigaction() failed for SIGALRM");
		reset_color();
	    exit(0);
	}

	// init variables for the sending
	init_state_sender();
	print_state_sender(state_send);
	do{
		if(byte_reads >= PACKET_SIZE)
			//try to send packets in pipelined
			byte_reads = reliable_send_datagram( buffer, size, sockfd, addr_ptr);

		//wait ack
		reliable_receive_ack( sockfd, addr_ptr);

		print_state_sender(state_send);
	}
	while( ackno != num_packet - 1 ); 
	//all datagram sent

	printf("Datagram sent with success\n");


}


