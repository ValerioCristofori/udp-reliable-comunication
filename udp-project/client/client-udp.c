/*
 * 		UDP Client
 * 		This client provide 3 command:
 * 			1- get command: provide the download of a specific 
 * 				file in the directory's tree.
 * 			2- put command: provide the upload of a specific 
 * 				file in the directory's tree.
 * 			3- list command: provide the list of the directory's tree.		
 *		All this features are allowed through reliabl edata transfer.  
 *
 */

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#include "defines.h"

#define SERV_PORT         2222
#define THRESHOLD         3
#define KEY               'S'


char *gets(char *s);


struct sockaddr_in       servaddr;
int                      sockfd;              // file descriptor
Datagram                *datagram_ptr;


void print_datagram( Datagram *datagram_ptr){

      printf("\nCommand: %s, sizeof(): %ld\n", datagram_ptr->command, sizeof(datagram_ptr->command) );
      printf("Filename: %s, sizeof(): %ld\n", datagram_ptr->filename, sizeof(datagram_ptr->filename) );
      printf("File size: %d, sizeof(): %ld\n", datagram_ptr->length_file, sizeof(datagram_ptr->length_file) );
      printf("File content: %s, sizeof(): %ld\n", datagram_ptr->file, sizeof(datagram_ptr->file) );
      printf("Error message: %s, sizeof(): %ld\n", datagram_ptr->error_message, sizeof(datagram_ptr->error_message) );
      printf("Code error: %d, sizeof(): %ld\n", datagram_ptr->err, sizeof(datagram_ptr->err) );
}

void print_file( char *file, int length ){
	
  /*
   *  Print the content of the 'file'
   *  Used to print the list of the files
   */
	
  char ch;

  for (int i = 0; i < length; i++) { 
        ch = file[i];
        printf("%c", ch ); 
        if (ch == EOF){ 
            return;
        } 
      
      }
  printf("\n");
}

char Cipher(char ch) 
{ 
	/*
	 * Return the encrypt or decrypt char matches with 'ch'
	 */
    return ch ^ KEY;  // the operator '^' make the XOR betw ch and KEY
					  // KEY must be a char 
}

int datagram_setup_put( Datagram* datagram_ptr, char** arguments,  FILE *fp ){
        
       /*
		* Return -1 if file is not supported 
		* its length is greater than MAXFILE
		* Else return the lenght of the datagram to send  
		*/
        
        int length, i;
        char ch, ch2;


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
        printf("command length: %ld\n", sizeof(datagram_ptr->command) );

        strcpy( datagram_ptr->filename, arguments[1] );
        datagram_ptr->err = 0;

        // build file of the datagram with file stream
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);  //count the file length
        fseek(fp, 0, SEEK_SET);
	
      	if( length >= MAXFILE ){
            printf("Length of file %d\n", length );
            return -1;
      	}	
        
        datagram_ptr->length_file = length;

        rewind(fp);  //rewind the file pointer to the start position

		//encrypt all char one by one and set in the datagram->file
        for ( i = 0; i < length; i++) {  

            ch = fgetc(fp); 
            ch2 = Cipher(ch); 
            datagram_ptr->file[i] = ch2; 
        }

        datagram_ptr->file[i] = EOF;  //add the end of file byte
        printf("File dimension %d\n", datagram_ptr->length_file);
        

        print_datagram(datagram_ptr);

        return LENGTH_HEADER + datagram_ptr->length_file;

}


int datagram_setup_get( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );  //set command string
        strcpy( datagram_ptr->filename, arguments[1] ); //set filename to get
        datagram_ptr->err = 0;		//no error
        datagram_ptr->length_file = 0;

        

        print_datagram(datagram_ptr);
        
        return LENGTH_HEADER + datagram_ptr->length_file;
        

} 

int datagram_setup_list( Datagram* datagram_ptr, char** arguments ){ 

        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );  //set command string
        datagram_ptr->err = 0;

        datagram_ptr->length_file = 0;

        print_datagram(datagram_ptr);


        return LENGTH_HEADER + datagram_ptr->length_file;
 
        

}

int datagram_setup_exit( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
        datagram_ptr->err = 0;

        datagram_ptr->length_file = 0;

        print_datagram(datagram_ptr);


        return LENGTH_HEADER + datagram_ptr->length_file;
 
        

}

int datagram_setup_exit_signal( Datagram* datagram_ptr){


        // setupping the struct 
        datagram_ptr->length_file = 0;
        datagram_ptr->err = 1;  //set error flag to 1 
								//exiting by signal
        

        print_datagram(datagram_ptr);


        return LENGTH_HEADER + datagram_ptr->length_file;
 
        

}


void handler_alarm_conn (int ign)  /* handler for SIGALRM */
{
	/*
	 * Exit from the client --> server not running
	 */
    printf("Error with the connection: server doesn't respond\n");
    exit(0);
}

void handler_sigint() { 
       
    int size;

        //send exit to server 
        //setting err to 1 in datagram
        //malloc datagram and clear
        datagram_ptr = malloc( sizeof(*datagram_ptr) );
        memset( datagram_ptr, 0, sizeof(*datagram_ptr));
        
        size = datagram_setup_exit_signal( datagram_ptr );

        printf("Start sender\n");
        start_sender(datagram_ptr, size, sockfd, &servaddr);
        

}

int stringCmpi(char *s1, char *s2)
{
    int i;
    for(i=0; s1[i]!='\0'; i++)
    {
        if( toupper(s1[i])!=toupper(s2[i]) )
            return 1;           
    }
    return 0;
}

void decrypt_content(FILE *fp, char* str, int length ){

      int     i;
      char    ch;

      
      for (i = 0; i<length; i++){ 
    
          // Input string into the file 
          // single character at a time 
          ch = Cipher(str[i]);
          fputc(ch, fp); 
     }
}


void path_to_filename( char *path , char *filename ){
	
	  /*
	   * Setup the char pointer 'filename' the name of the file
	   * parsing the path through strstr
	   */


      int l=0;
      char* ssc;
      
      ssc = strstr(path, "/");
      do{
          l = strlen(ssc) + 1;
          path = &path[strlen(path)-l+2];
          ssc = strstr(path, "/");
      }while(ssc);

      strcpy( filename, path );
}


void manage_error(Datagram *datagram_ptr, int sockfd ){
      
      /*
       *  Handler for the error messages
       *  through error code and the cases
       *  define in the file defines.h
       */
      
      switch(datagram_ptr->err){


          case 2:

                printf("%s\n", ERROR_FILE_DOESNT_EXIST );
                break;

          case 3:

                printf("%s\n", ERROR_SHELL_SCRIPT );
                break;

          case 4:

                printf("%s\n", ERROR_SIGINT_SERVER );
                exit(-1);

          case 5:

                printf("%s\n", ERROR_TOO_MANY_MATCHES );
				break;


      }

}





int main(int argc, char *argv[]) {
  int                      maxfd;								// number of byte receved from recvfrom or sendto
  int                      n, tmp, size;
  fd_set                   fds;                 // set di descrittori
  char                     buff[MAXLINE];        // input string of the user
  char                     *ptr;
  socklen_t                len;
  char                   **arguments = NULL;
  FILE*                    fp;
  char                    *dirs;
  char                     command[FILENAME_LENGTH + 16];
  int                      fd;
  short                    syn = 0x10;
  int                      client_port;
  struct sigaction         sa;


  




  if (argc != 2) { /* controlla numero degli argomenti */
    fprintf(stderr, "howtouse: ./client-udp <indirizzo IP server>\n");
    exit(1);
  }

  /* handler a sigalarm for bad connection */
  sa.sa_handler = handler_alarm_conn;
  if (sigfillset (&sa.sa_mask) < 0){ /* block everything in handler */
      perror("sigfillset() failed");
      exit(0);
  }
  sa.sa_flags = 0;

    if (sigaction (SIGALRM, &sa, 0) < 0){
    perror("sigaction() failed for SIGALRM");
      exit(0);
  }

  alarm(THRESHOLD); //alarm signal if server dont respond
					//until threshold sec 

  
  memset((void *)&servaddr, 0, sizeof(servaddr));      /* set 0 servaddr struct */
  
  sockfd = udp_socket_init_client( &servaddr, argv[1], SERV_PORT );
  if( sockfd == -1 ){
    perror("socket init error");
    exit(-1);
  }



  printf("Connection to the server on address %s and port %d\n", argv[1], SERV_PORT );


  FD_ZERO(&fds);  /* Init with 0 all the entry of fds set */

  //send del pacchetto di syn per richiedere una porta
  printf("Syn sent\n");

  n = sendto( sockfd ,  &syn,  sizeof(syn) , 0 ,
              ( struct sockaddr *)&servaddr , sizeof( servaddr ));
  if ( n < 0 ) {
    perror ( " sendto error " );
    exit ( -1);
  }
  len = sizeof(servaddr);
  //Wait the new port no 
  n = recvfrom( sockfd ,  &tmp,  sizeof(tmp),  0, (struct sockaddr *)&servaddr,  &len );
  if( n <= 0 ){
      perror( "recvfrom error" );
      exit(-1);
  }
  client_port = ntohl(tmp); //setting the new port number for new socket 
  
  alarm(0);   // reset alarm --> good connection

  //change server port with the negotiate one
  memset((void *)&servaddr, 0, sizeof(servaddr));
  
  sockfd = udp_socket_init_client( &servaddr, argv[1], client_port );
  if( sockfd == -1 ){
    perror("socket init error");
    exit(-1);
  }

  printf("New socket with %d port number\n", ntohs(servaddr.sin_port) );

  //set the handler for sigint signal
  sa.sa_handler = handler_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
      printf("sigaction error\n");
      exit(-1); 
  }


  while(1){
    
    //-----------------------   Client body
          printf("\nType something:\n");

          /* use fileno() for conversion from FILE* to int */
          FD_SET( fileno(stdin), &fds );  /* add stdin file descriptor */
          FD_SET( sockfd, &fds );           /* add socket with server descriptor */
          

          maxfd = (fileno(stdin) < sockfd) ? (sockfd + 1): (fileno(stdin) + 1);  /* maxfd begin the max file descriptor + 1 */

          if (select(maxfd, &fds, NULL, NULL, NULL) < 0 ) { /* wait a descriptor ready in reading */
                perror("select error");
                exit(1);
          }

          /* Serve the descriptor that cause the exit from select call */

          /* check if stdin is readable */
          if (FD_ISSET(fileno(stdin), &fds)) {   /*  check if stdin is in the set of fds */
               
              memset(&buff, 0, sizeof(buff));

              if (fgets(buff, sizeof(buff), stdin) != NULL)    // get the input from the command line
              {
                if ((ptr = strchr(buff, '\n')) != NULL)
                  *ptr = '\0';
                
              }
              printf("il buffer contiene: %s\n",  buff);
              arguments = str_split(buff, ' ');/* parsing the shell command */
     
              if( arguments == NULL ){
                  printf("\nWrong arguments\nTry to type:\nput <path/filename>\nget <path/filename>\nlist\nexit\n");
                  goto retry;
              }
              

              //malloc datagram and clear
              datagram_ptr = malloc( sizeof(*datagram_ptr ));
              memset( datagram_ptr, 0, sizeof(*datagram_ptr));



              if( strcmp(arguments[0], "put") == 0 ){

                              


                              //------------------- put body --------------------------
                              


                              if( arguments[1] == NULL ){
                                  perror("You must insert the name of the file.");
                                  goto retry;
                              }

                              printf("put\n" );

                              

                              fp = fopen( arguments[1], "r"); 
                              printf("\nFile Name Received: %s\n", arguments[1]); 
                              if (fp == NULL){ 
                                  printf("\nFile open failed!\nFile does not exist.\n"); 
                                  goto retry;
                              }
                              else
                                  printf("\nFile Successfully opened!\n"); 

                              size = datagram_setup_put(  datagram_ptr,  arguments,  fp  );
                              if( size == -1 ){
                                  printf("Error: file too large\nSend file not greater than %d\n", MAXFILE);
                                  goto retry;
                              }

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr); /* Send the request to the server */


              }else if( strcmp(arguments[0], "get") == 0 ){
                

                              //------------------- get body --------------------------

                              if( arguments[1] == NULL ){
                                  perror("You must insert the name of the file.");
                                  goto retry;
                              }

                              printf("get\n");

                              size = datagram_setup_get( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr); /* Send the request to the server */

                              //clear datagram
                              memset( datagram_ptr, 0, sizeof(*datagram_ptr));

                              printf("Start receiver\n");
                              size = start_receiver(datagram_ptr, sockfd, &servaddr, 0.1); /* Wait the response from the server */
                              if(size == -1){
                                    manage_error(datagram_ptr, sockfd);
                                    goto retry;
                              }


                              //datagram received
                              printf("Datagram ricevuto (comando get)\n");
                                                          
                              /*
                               *  Parsing the datagram and
                               *  open a file with filename datagram->filename
                               */

                              if( strchr(datagram_ptr->filename, '/') != NULL){  //check if the program have to make some dirs

                                    //take the name of all the dirs
                                    //deleting the last chars until '/'
                                    printf("Build directories\n");
                                    dirs = malloc( sizeof(datagram_ptr->filename) );
                                    build_directories(datagram_ptr->filename, dirs);

                                    //run the shell script for making directory/ies
                                    sprintf(command, "./script-shell/make_dirs.sh %s", dirs);
                                    system(command);


                            
                              }

                              printf("filename: %s\n", datagram_ptr->filename);
                          

                              if ((fd = open( datagram_ptr->filename, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
                                  printf("Error opening file %s\n", datagram_ptr->filename);
                                  exit(-1);
                              }

                              if ((fp = fdopen(fd,"w+")) == NULL) {
                                  printf("Error fopening file %s\n", datagram_ptr->filename);
                                  exit(-1);
                              }

                              decrypt_content(fp, datagram_ptr->file, datagram_ptr->length_file);
                              fflush(fp);

                              printf("Success on download file %s\n", datagram_ptr->filename );








                
              }else if( strcmp(arguments[0], "list") == 0 ){

                


                              //------------------- list body --------------------------


                              printf("list\n" );

                              size = datagram_setup_list( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr);  /* Send the request to the server */


                              //clear datagram
                              //---------------------------------------
                              //free(datagram_ptr);

                              printf("Start receiver\n");
                              size = start_receiver(datagram_ptr, sockfd, &servaddr, 0.1);   /* Wait the response from the server */
                              if(size == -1){
                                    manage_error(datagram_ptr, sockfd);
                                    goto retry;
                              }
                              printf("Bytes received %d\n", size );
                              print_datagram(datagram_ptr);


                              printf("List of the files: %s\n", datagram_ptr->file );  //print the tree of the files
                              
                       
                
              }else if( strcmp(arguments[0], "exit") == 0 ){

                              printf("Exiting...\n"); 
                              

                              size = datagram_setup_exit( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr); /* Send exit message to the server */

                              free(datagram_ptr);
                              free(arguments);           
                              break;


              }else{
                 
                  printf("\nWrong arguments\nTry to type:\nput <path/filename>\nget <path/filename>\nlist\nexit\n");
                  continue;
              }
              
            retry:
              free(datagram_ptr);
              free(arguments);

          }



  }

  
  
  // normal exit
  exit(0);
}
