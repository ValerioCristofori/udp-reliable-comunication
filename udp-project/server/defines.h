#pragma once

#define MAXFILE           4096
#define FILENAME_LENGHT   32
#define TIMEOUT			  3
#define MAXTRIES          10
#define MAXLINE           1024
#define PACKET_SIZE    	  256

/* datagram struct */
typedef struct datagram_value {
      
      char command[5];
      char filename[FILENAME_LENGHT];
      char file[MAXFILE];
      char error_message[32];
      int  die_sig;
      int  err;

} Datagram;


/* state of communication */
typedef struct state_communication {

	   int window;
	   int tries;
	   int send_base;
	   int next_seq_no;
	   int packet_sent; 
	   int expected_seq_no;

} State;


/* packet struct for go back n transport */
typedef struct gobackn_packet{

	   int seq_no;
	   int length;
	   char data[256];
}Packet;

/* utils functions */
extern ssize_t FullWrite ( int fd , const void * buf , size_t count );

extern int udp_socket_init_server( struct sockaddr_in*   addr,  char*   address, int   num_port );

extern int udp_socket_init_client( struct sockaddr_in*   addr,  char*   address, int   num_port );

extern char** str_split(char* a_str, const char a_delim);

extern int split( char *str, char c, char ***arr );

extern void start_sender( Datagram* datagram, int sockfd, struct sockaddr_in* addr_ptr );

extern void start_receiver( Datagram* datagram, int sockfd, struct sockaddr_in* addr_ptr, double prob_loss );