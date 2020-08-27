#include "defines.h"


void init_state_receiver(State *s){
	/*
	 *  Init the struct State connection
	 *  for the specific couple client - thread server
	 */
	s->expected_seq_no = 0;
	s->ack_no = -1;
}


int reliable_receive_packet(State *s, char  *buffer, int sockfd, struct sockaddr_in * addr_ptr){

	/*
	 *	Return the number of bytes received
	 *  The data loss is simulated by the func drand48 and an input prob_loss  
	 */

	Packet 			current_packet;
	int 			n;
	int 			rnd;
	socklen_t 		len;

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


	rnd = rand() % 100;
	if( rnd < prob_loss ){    //simulation of loss
		bold_red();
		printf("---------------- PACKET LOST ---------------\n");
		reset_color();
		return 0;
	}
	green();
	printf (">>> Received packet %d with length %d\n", current_packet.seq_no, current_packet.length);

	// Send ack and store in buffer	
  	if (current_packet.seq_no == s->expected_seq_no )   //check if is the correct packet comparing labels
    {
      	
      	memcpy (&buffer[PACKET_SIZE * current_packet.seq_no], current_packet.data, current_packet.length);
      	s->ack_no = current_packet.seq_no;
      	s->expected_seq_no++;

    }else{
    	current_packet.length = 0;
    }
    printf (">>> Send ack %d\n", s->ack_no );
    reset_color();
    //Send the ack with lable ack_no
    Packet current_ack; // ack 
	current_ack.seq_no = htonl(s->ack_no );
	current_ack.length = htonl(0);

	if (sendto(sockfd, &current_ack, sizeof(current_ack), 0,(struct sockaddr *) addr_ptr,
		       sizeof (*addr_ptr)) !=  sizeof(current_ack) ){
				
				red();
		    	perror(" sent a different number of bytes ");
		    	reset_color();
		    	exit(1);
	}


	return current_packet.length;  //return 0 in cases: data loss of wrong ack  

}


int start_receiver( Datagram *datagram, int sockfd, struct sockaddr_in * addr_ptr){


		/*
		 *  Return the dimension of datagram (bytes received)
		 *  Function to start the receiving packets to sockaddr_in
	     *  Implemented the go back n protocol for reliable data transfer
	     *  Used the probability p to simulate the data loss and
	     *  check the reliability.
		 *
		 *  After the receiving all the structured buffer is copied in a
		 *  datagram struct 
		 */

		void* buffer;
		State  *s;
		int   datasize = sizeof(*datagram);
		int   size = 0, n;
		buffer = malloc( datasize );
		s = malloc(sizeof(*s));

		init_state_receiver(s);
		printf("Start receiving bytes\n");

		while( (n = reliable_receive_packet(s, buffer, sockfd, addr_ptr) ) == PACKET_SIZE || n == 0  ){
			size += n;
		}
		size += n;


		printf("Bytes received %d\n", size );

		memcpy( datagram, buffer, size);

		if( datagram->err ){  // there are some signal error
			return -1;
		}

		return size;



}