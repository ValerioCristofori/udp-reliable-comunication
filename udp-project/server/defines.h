#pragma once
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/in.h>

#define MAXFILE           16777216       // 2^24 bytes ----- 16 MB
#define FILENAME_LENGTH   32				
#define COMMAND_LENGTH    5				 //Lenght of the command client to server
#define LENGTH_HEADER     85			 //Lenght of the datagram's header
#define ERROR_MESSAGE_LENGTH  32
#define MAXTRIES          10			 //Upper bound of tries to send without good ack
#define MAXLINE           1024			 //Max bytes take from input
#define PACKET_SIZE    	  256			 //Dimension of the single packet in go back n protocol
#define MAX_THREADS       20			 //Max thread opened in the same time for manage connections
#define KEY          	  'S'			 //Key for encrypt/decrypt_content (using xor operator char by char)
#define TIMER 			  180			 //Timer until the exiting from the server if client doesnt respond

#define red() 			printf("\033[0;31m")
#define bold_red() 		printf("\033[1;31m")
#define blue()			printf("\033[0;34m")
#define green()			printf("\033[0;32m")
#define yellow()		printf("\033[01;33m")
#define cyan()			printf("\033[0;36m")
#define reset_color()   printf("\033[0m")

#define ERROR_SIG_CLIENT   	     "Error: Client finished through signal\nExiting from the thread child.\n"             //case 1
#define ERROR_FILE_DOESNT_EXIST  "Error: File does not exist.\n"													   //case 2
#define ERROR_SHELL_SCRIPT 		 "Error: Server is working bad.\n"													   //case 3
#define ERROR_SIGINT_SERVER      "Error: Server finished through signal CTRL+C.\n"									   //case 4
#define ERROR_TOO_MANY_MATCHES   "Error: Too many matches with the input filename.\nRetry more specific request.\n"    //case 5


/* datagram struct */
typedef struct datagram_value {
      
      	char command[COMMAND_LENGTH];  				//specific operation: put, get, list, exit
      	char filename[FILENAME_LENGTH];				//name of the file in put and get cases
      	char error_message[ERROR_MESSAGE_LENGTH];		//type of error : case 1-5
      	int  err;										//flag for error
      	int  length_file;								//length data part
      	char file[MAXFILE];  							//data

} Datagram;


/* state of communication */
typedef struct state_communication {

	   	int tries;					//counter for the current tries
	   	int send_base;				//label of the last packet not yet acked
	   	int next_seq_no;				//label of next packet to send
	   	int packet_sent; 			//label current packet sent
	   	int expected_seq_no;			//expected label for the received packet
	   	int ack_no;					//ack number 
	   	int window_ack;				//window of ack received
	   	int byte_reads;				//bytes reads -> used for critical case

} State;

/* relationship between thread server and its state */
typedef struct relationship {
	
	   /*
	    *  Implemented for manage the alarm signal 
	    *  for a timeout for the exact thread
	    */

	   	pthread_t th;		//thread server child that response to a client
	   	State *state;		//the state of that specific connection

} R;


/* packet struct for go back n transport */
typedef struct gobackn_packet{

	   	int seq_no;			//label of the packet
	   	int length;			//effective length of the packet, max 256
	   	char data[PACKET_SIZE];      //data
	   
}Packet;


extern int 	 timeout;	//Timeout of the go back n protocol
extern int 	 server_port;  //Num port where main thread wait connections
extern int 	 window;    //the dimension of the window(packets in fly)
extern int   prob_loss;


extern R **relations;   //global resource in server.c file


/* utils functions */

extern int parse_argv( char **argv );

extern int udp_socket_init_server( struct sockaddr_in  *addr,  char  *address, int   num_port, int option );

extern int generate_random_num_port();

extern void build_directories( char *path, char *dirs );

extern int filename_to_path( char* filename, char* path );

/* gbn functions */

extern void start_sender( Datagram *datagram, int size, int sockfd, struct sockaddr_in *addr_ptr, pthread_t whoami );

extern int start_receiver( Datagram *datagram, int sockfd, struct sockaddr_in *addr_ptr); /* return number bytes read */

/* server functions */

extern void thread_death();


/* datagram setup functions */

extern void decrypt_content(FILE *fp, char *str, int length );

extern int datagram_setup_get( Datagram *datagram, char *filename );

extern int datagram_setup_list( Datagram *datagram);

extern int datagram_setup_error( Datagram *datagram, int error );


/* print functions */

extern void print_datagram( Datagram *datagram);

extern void print_state_sender(State *s);
