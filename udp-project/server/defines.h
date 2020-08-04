#pragma once

#define MAXFILE           4096
#define FILENAME_LENGTH   32
#define ERROR_MESSAGE_LENGTH  32
#define TIMEOUT			  3
#define MAXTRIES          10
#define MAXLINE           1024
#define PACKET_SIZE    	  256
#define MAX_THREADS       10

#define ERROR_SIGINT_CLIENT   	 "Error: Client finished through signal CTRL+C\nExiting from the thread child.\n"      //case 1
#define ERROR_FILE_DOESNT_EXIST  "Error: File does not exist.\n"													   //case 2
#define ERROR_SHELL_SCRIPT 		 "Error: Server is working bad.\n"													   //case 3
#define ERROR_SIGINT_SERVER      "Error: Server finished through signal CTRL+C.\n"									   //case 4
#define ERROR_TOO_MANY_MATCHES   "Error: Too many matches with the input filename.\nRetry more specific request.\n"    //case 5


/* datagram struct */
typedef struct datagram_value {
      
      char command[5];
      char filename[FILENAME_LENGTH];
      int  length_filename;
      int  datagram_size;
      int  length_file;
      char file[MAXFILE];  
      char error_message[ERROR_MESSAGE_LENGTH];
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
	   int ack_no;
	   int window_ack;
	   int byte_reads;

} State;

/* relationship between thread server and its state */
typedef struct relationship {

	   pthread_t th;
	   State *state;

} R;


/* packet struct for go back n transport */
typedef struct gobackn_packet{

	   int seq_no;
	   int length;
	   char data[256];
}Packet;

extern R **relations;

/* utils functions */
extern ssize_t FullWrite ( int fd , const void * buf , size_t count );

extern int udp_socket_init_server( struct sockaddr_in*   addr,  char*   address, int   num_port );

extern int udp_socket_init_client( struct sockaddr_in*   addr,  char*   address, int   num_port );

extern char** str_split(char* a_str, const char a_delim);

extern int split( char *str, char c, char ***arr );

extern void start_sender( Datagram* datagram, int size, int sockfd, struct sockaddr_in* addr_ptr, pthread_t whoami );

extern int start_receiver( Datagram* datagram, int sockfd, struct sockaddr_in* addr_ptr, double prob_loss ); /* return number bytes read */
