/*
 * 		Concurrent UDP Server
 * 		This server provide 3 command:
 * 			1- get command: provide the download of a specific 
 * 				file in the directory's tree.
 * 			2- put command: provide the upload of a specific 
 * 				file in the directory's tree.
 * 			3- list command: provide the list of the directory's tree.		
 *		All this features are allowed through reliable data transfer.  
 *
 *
 */

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

#include "defines.h"

#define SERV_PORT          2222
#define KEY          	   'S'


int                        numthreads   = 0;     // Number of thread open for conns                      
pthread_mutex_t            mut          = PTHREAD_MUTEX_INITIALIZER;   //  Mutex for upgrade numthread resource
R                        **relations;		     // Ptr to the list of relations between one thread and its sender state



int stringCmpi(char *s1,char *s2)
{
	/*
	 * Return 0 if s1 and s2 match also without
	 * case sensitive
	 * 
	 * Return 1 else
	 */
	
    int i=0;
    for(i=0; s1[i]!='\0'; i++)
    {
        if( toupper(s1[i])!=toupper(s2[i]) )
            return 1;           
    }
    return 0;
}


char Cipher(char ch) 
{ 
	/*
	 * Return the encrypt or decrypt char matches with 'ch'
	 */
    return ch ^ KEY;  // the operator '^' make the XOR betw ch and KEY
					  // KEY must be a char 
}


void print_datagram( Datagram *datagram_ptr){

      printf("\nCommand: %s, sizeof(): %ld\n", datagram_ptr->command, sizeof(datagram_ptr->command) );
      printf("Filename: %s, sizeof(): %ld\n", datagram_ptr->filename, sizeof(datagram_ptr->filename) );
      printf("File size: %d, sizeof(): %ld\n", datagram_ptr->length_file, sizeof(datagram_ptr->length_file) );
      printf("File content: %s, sizeof(): %ld\n", datagram_ptr->file, sizeof(datagram_ptr->file) );
      printf("Error message: %s, sizeof(): %ld\n", datagram_ptr->error_message, sizeof(datagram_ptr->error_message) );
      printf("Code error: %d, sizeof(): %ld\n", datagram_ptr->err, sizeof(datagram_ptr->err) );
      
}


void print_buff_datagram( Datagram* datagram_ptr ){
	
      printf("command : %s, filename: %s\n", datagram_ptr->command, datagram_ptr->filename );
      
}

void thread_death(){
	/*
	 * Take the critical section
	 * upgrade the number because thread's death
	 */
    pthread_mutex_lock(&mut);
    numthreads--;
    pthread_mutex_unlock(&mut);
}

void thread_birth(){
	/*
	 * Take the critical section
	 * upgrade the number because thread's birth
	 */
    pthread_mutex_lock(&mut);
    numthreads++;
    pthread_mutex_unlock(&mut);  
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


int datagram_setup_get( Datagram* datagram_ptr, char* filename ){
   
   /*
    * Return -1 if cant open the file
    * Else return the lenght of the datagram to send  
    */
   
   
   FILE*      fp;
   int        length, i;
   char       ch, ch2;

			

          fp = fopen( filename, "r");   //try to open the file
          printf("\nFile Name Received: %s\n", filename); 
          if (fp == NULL){ 
              printf("\nFile open failed!\n"); 
              return -1;
          }
          else
              printf("\nFile Successfully opened!\n");

          // build file of the datagram with file stream
          fseek(fp, 0, SEEK_END);
          length = ftell(fp);  //count the file length
          fseek(fp, 0, SEEK_SET);
          datagram_ptr->length_file = length;

          rewind(fp); //rewind the file pointer to the start position

		  //encrypt all char one by one and set in the datagram->file
          for ( i = 0; i < length; i++) {  
			
              ch = fgetc(fp);  
              ch2 = Cipher(ch); 
              datagram_ptr->file[i] = ch2; 
          }
 
          datagram_ptr->file[i] = EOF;  //add the end of file byte
          printf("File dimension %d\n", datagram_ptr->length_file);

          return LENGTH_HEADER + datagram_ptr->length_file;
}


int datagram_setup_list( Datagram* datagram_ptr){

	/*
	 * Very similar to the get setup case
	 * 
     * Return -1 if cant open the file
     * Else return the lenght of the datagram to send  
     */

    FILE* fp;
    char ch;
    int length;
    int i;



          printf("Open file that contains tree\n");
          fp = fopen( "tree", "r");  // try to open the file of the dirs, 'tree'
          if (fp == NULL) {
              printf("\nFile open failed!\n"); 
			  return -1;
          }else
              printf("\nFile Successfully opened!\n");

          fseek(fp, 0, SEEK_END);
          length = ftell(fp);
          fseek(fp, 0, SEEK_SET);

          datagram_ptr->length_file = length;

          rewind(fp);
			
		  
          for ( i = 0; i < length; i++) { 
              ch = fgetc(fp);  
              datagram_ptr->file[i] = ch; 
          }
          printf("File dimension %d\n", datagram_ptr->length_file);
          
          return LENGTH_HEADER + datagram_ptr->length_file;

    

}

int datagram_setup_error( Datagram *datagram_ptr, int error ){

	  /*
	   * Used to say at the client the error state of his call
	   * Return the length of the datagram for the sending
	   */

      // setupping the struct 
      datagram_ptr->length_file = 0;
      datagram_ptr->err = error;     //init the datagram entry 'err' with the specific error code 
									 //defined in the file defines.h

      print_datagram(datagram_ptr);


      return LENGTH_HEADER + datagram_ptr->length_file;
}


int filename_to_path( char* filename, char* path ){
    
    /*
     * Setup in in the char pointer 'path' the path of the file
     * in the directories's tree , using the shell script 'search_dir.sh'
     * 
     * Return  1 if the executing of shell script is failed
     * Return -1 if the file doesnt exist
     * Return -2 if the are too many matches with the filename 'filename'
     * Return  0 else
     */
    
    FILE *p;
    char command[FILENAME_LENGTH + 16];
    char buffer[MAXLINE];

    sprintf(command, "./search_dir.sh %s", filename);
    printf("%s is the command\n", command );

    p = popen(command, "r");  /* launching shell script and creating a pipe 
										for the results */
    
    if (p != NULL) {
      fscanf(p, "%[^\n]", buffer);
      if( strstr(buffer, filename) == NULL ){
          printf("file doesnt exist\n");
          return -1;
      }
      if( strstr(buffer, "./root/") != NULL ){
          printf("too many file matches with this filename\n");
          return -2;
      }  
      printf("result of the shell script: %s\n", buffer );
      strcpy(path, buffer); //path points to buffer head
      pclose(p);
    }
    else{
      printf("error on executing shell script\n");
      return 1;
    }
    return 0;
}


void manage_error_signal(Datagram *datagram_ptr, int sockfd ){
	
	/*
	 * Called when the server received a datagram with an error
	 * If the error is 1 --> the server close the socket with the specific client 
	 * and close the thread that manage that connection
	 */
      
      switch(datagram_ptr->err){

          case 1:
          default:
                
                printf("The client finished through signal\nExiting from the thread child\n" );
                thread_death();
                close(sockfd);
                pthread_exit(NULL);


      }

}




int generate_random_num_port(){
  
  /*
   *  Generate a random number betw 'lower' and 'upper'
   * 		This is the portno of a connection
   * 		When the conn is closed the number is reused 
   * 		through the exception of the binding in the socket
   */
  
  int lower = 2224;
  int upper = 5000;

  return ( rand() % ( upper - lower + 1 ) + lower );

}




void *client_request( void *sockfd ){

  /*
   * 
   * 	Thread opened by the main thread every conn request
   * 	This thread manage all the request of a client
   * 		-put
   * 		-list
   * 		-get
   * 		-exit
   * 		-error
   * 
   */


  struct  sockaddr_in     clientaddr, relation;
  Datagram                *datagram_ptr;
  int                     n, sock, sock_data, fd;
  int                     client_port, tmp, ret, size;
  socklen_t               len;
  FILE*                   fp;
  pthread_t               whoami = pthread_self();
  char                    path_file[40];
  char                    command[FILENAME_LENGTH + 16];
  char                    path[MAXLINE];
  char                    *dirs;
  short                   syn;
  

        printf("Created a thread handler\n");  
        pthread_detach(whoami);    /* when a detached thread terminates releases resources */
        thread_birth();

        memcpy(&sock,sockfd,sizeof(sock));
        len = sizeof(relation);

        n = recvfrom( sock,  &syn,  sizeof(syn),  0, (struct sockaddr *)&relation,  &len );
        if( n <= 0 ){
            perror( "recvfrom error" );
            thread_death();
            pthread_exit(NULL);
        } 
        printf("Syn received: %u\n", syn );

    retry_num_port:  //label used when i try to use a portno that already exist

        client_port = generate_random_num_port();
        printf("%d\n", client_port );

        // create the udp socket 
        sock_data = udp_socket_init_server( &clientaddr, NULL, client_port, 0 );
        if( sock_data == -1 ){
            thread_death();
            perror("socket init error");
            pthread_exit( NULL );
        }else if( sock_data == 0 ){
            printf("Try to use a port already used\n");
            goto retry_num_port;
        }

        //send the new port number to the client
        tmp = htonl(client_port);
        n = sendto( sock ,  &tmp,  sizeof(tmp) , 0 ,
                    ( struct sockaddr *)&relation , sizeof( relation ));
        if ( n < 0 ) {
            perror ( " sendto error " );
            thread_death();
            pthread_exit(NULL);
        }

        


        while(1){
            
            //malloc datagram and clear
            datagram_ptr = malloc( sizeof(*datagram_ptr) );
            memset( datagram_ptr, 0, sizeof(*datagram_ptr));

			//Wait for the request from the client
            printf("Start receiver\n");
            size = start_receiver(datagram_ptr, sock_data, &clientaddr, 0.1);  // call blocker
            if( size == -1 ){
                  manage_error_signal(datagram_ptr, sock_data);
            }

            print_datagram(datagram_ptr);

            if( strcmp( datagram_ptr->command, "put") == 0 ){   //search the match with datagram->command

                        

                        //------------------- put body --------------------------
                        

                        printf("put\n" );
                        sprintf(path_file, "root/%s", datagram_ptr->filename);

                        if( strchr(datagram_ptr->filename, '/') != NULL){  //check if the program have to make some dirs

                              //take the name of all the dirs
                              //deleting the last word until '/'
                              printf("Build directories\n");
                              dirs = malloc( sizeof(datagram_ptr->filename) );
                              build_directories(datagram_ptr->filename, dirs);

                              //run the shell script for making directory/ies
                              sprintf(command, "./make_dirs.sh %s", dirs);
                              system(command);


                            
                        }
                        //creating the file with 'path' name

                        if ((fd = open( path_file, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
                            printf("Error opening file %s\n", path_file);
                            thread_death();
                            close(sock_data);
                            pthread_exit(NULL);
                        }

                        if ((fp = fdopen(fd,"w+")) == NULL) {
                            printf("Error fopening file %s\n",path_file);
                            thread_death();
                            close(sock_data);
                            pthread_exit(NULL);
                        }

                        decrypt_content(fp, datagram_ptr->file, datagram_ptr->length_file); //build the file descriptying content
                        fflush(fp);
                        printf("Success on download file %s\n", datagram_ptr->filename );


                        free(datagram_ptr);
                        





            }else if( strcmp(datagram_ptr->command, "get") == 0 ){
              
                        

                        //------------------- get body --------------------------


                        printf("get\n");
                        //check if the file exist in the root dir and subdirs
                        //if y setup datagram
                        //if n setup error in datagram
                        ret = filename_to_path(datagram_ptr->filename, path);
                        if( ret == 1 ){
                            printf("send error message to the client\n");
                            size = datagram_setup_error( datagram_ptr, 3 );

                        
                        }else if( ret == -1 ){
                            printf("send error message to the client\n");
                            size = datagram_setup_error( datagram_ptr, 2 );
                            

                        }else if( ret == -2 ){

                            printf("send error message to the client\n");
                            size = datagram_setup_error( datagram_ptr, 5 );

                        }else{
                            printf("path file: %s\n", path );
                            if( (size = datagram_setup_get( datagram_ptr, path ) ) == -1 ){  /* -1 if file doesn't exist */
                                thread_death();
                                close(sock_data);
                                pthread_exit(NULL);
                            }else{

                                printf("Il contenuto del file '%s': %s\n", path, datagram_ptr->file );

                            }

                        }  
                        
                        printf("Start sender\n");
                        start_sender(datagram_ptr, size, sock_data, &clientaddr, whoami);


                        free(datagram_ptr);
                      




              
            }else if( strcmp(datagram_ptr->command, "list") == 0 ){

                        

                        //------------------- list body --------------------------


                        printf("list\n" );


                        /*
                         *  launch the shell script that build a file called 'tree'
                         * 	that contains the structure of th subdirectories of ./root/
                         */
                        printf("Lauch shell script\n");
                        system("./build_tree.sh"); // launch shell script

                        size = datagram_setup_list( datagram_ptr );


                        printf("Start sender\n");
                        start_sender(datagram_ptr, size, sock_data, &clientaddr, whoami);

                        free(datagram_ptr);

              
            }else if( strcmp(datagram_ptr->command, "exit") == 0 ){

                        //------------------- exit body --------------------------


                        printf("exit\n");


                        printf("Exiting from thread child...\n");
                        //close the connection and free resources
                        thread_death(); 
                        close(sock_data);
                        pthread_exit(NULL);

            }
            else{

              perror("error with the arguments passed\n");
              thread_death();
              close(sock_data);
              pthread_exit(NULL);
            }
        }      

        printf("Exiting from thread child...\n");
        //dead code
        //close the connection and free resources
        thread_death();
        close(sock_data);
        pthread_exit(NULL);


}



int main(int argc, char *argv[]) {
  
  int	                  sockfd; 						//file descriptor
  fd_set                  sockets;
  struct sockaddr_in	  servaddr;
  pthread_t               threads[MAX_THREADS];
  



      // clear the address
      memset((void *)&servaddr, 0, sizeof(servaddr));
      // create the udp socket 
      sockfd = udp_socket_init_server( &servaddr, NULL, SERV_PORT, 1 );
      if( sockfd == -1 ){
        perror("socket server init error");
        exit(-1);
      }

     
      /* 
       *  finished bind between socket and address
       *  Server can read on this socket
       */


      FD_ZERO(&sockets);         /* Init with 0 all the entry of fds set */
      FD_SET(sockfd,&sockets);   /* Add socket in the set , setting to 1 the entry */
      fcntl(sockfd,F_SETFL,O_NONBLOCK);    /* recv not blocking */


      //malloc the struct for connection 
      relations = malloc(MAX_THREADS*sizeof(R *));
      

      while (1) {
		  
              //    -------------------    Server body Main thread    -------------------------
              
              /* 
               * Check on reading the state of the sockets 
               * Multiplexing I/O
               * The server can manage all the requests with an unique flow (main)
               * The management of the single connection is delegated to a specific thread child
               * 
              */


              if ( select(sockfd + 1,&sockets,NULL,NULL,NULL) < 0) {
                  perror("select()");
                  return  -1;
              }


              /*
               *  Serve the client that cause the exit from select call
               *  by creating a new thread child that manages the requests
               */
                               
              if (numthreads < MAX_THREADS) {
				  //Selected a upper bound for threads 'MAX_THREADS'
                  pthread_create(&threads[numthreads],NULL, client_request ,(void *)&sockfd);
              }
              printf("Currently handling client. %d threads in use.\n",numthreads);   
      }
      
      //normal exit
      exit(0);
}
