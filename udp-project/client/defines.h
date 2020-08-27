#pragma once
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define KEY               'S'			 //key for encrypt/decrypt
#define THRESHOLD         3				 //number of seconds after the connection is closed if server doesnt respond
#define ALPHA			  0.125
#define BETA 			  0.25

#define red() 			printf("\033[0;31m")
#define bold_red()      printf("\033[1;31m")
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


/* datagram struct *///
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

} State;


/* packet struct for go back n transport */
typedef struct gobackn_packet{

	   	int 	seq_no;			//label of the packet
	   	int 	length;			//effective length of the packet, max 256
	   	char 	data[PACKET_SIZE];      //data
	   
}Packet;


extern int 	 	timeout;		//Timeout of the go back n protocol
extern int 	 	server_port;  	//Num port where main thread wait connections
extern int 	 	window;    		//the dimension of the window(packets in fly)
extern int 	    prob_loss;      //probability of loss
extern int 		adaptive;		//variable for adaptive timer if it isnt 0



/* utils functions */

extern int parse_argv( char **argv );

extern int udp_socket_init_client( struct sockaddr_in  *addr,  char  *address, int   num_port );

extern char** str_split(char *a_str, const char a_delim);

extern int change_adaptive_timer(struct timeval time_begin, struct timeval time_end, double *estimate_RTT, double *sample_RTT, double * array_estimate, int *index, int *count);

extern int reset_adaptive_timer(double *estimate_RTT, double *sample_RTT, double * array_estimate, int *index, int *count);

extern void build_directories( char *path, char *dirs );

/* gbn functions */

extern void start_sender( Datagram *datagram, int size, int sockfd, struct sockaddr_in *addr_ptr );

extern int start_receiver( Datagram *datagram, int sockfd, struct sockaddr_in *addr_ptr); /* return number bytes read */


/* print functions */

extern void print_datagram( Datagram *datagram );

extern void print_file( char *file, int length );

extern void print_error_datagram(Datagram *datagram, int sockfd );

extern void print_state_sender(State  *state_send);


/* datagram setup functions */

extern void decrypt_content(FILE *fp, char *str, int length );

extern int datagram_setup_put( Datagram * datagram, char **arguments,  FILE *fp );

extern int datagram_setup_get( Datagram *datagram, char **arguments );

extern int datagram_setup_list( Datagram *datagram, char **arguments );

extern int datagram_setup_exit( Datagram *datagram, char **arguments );

extern int datagram_setup_exit_signal( Datagram *datagram);

