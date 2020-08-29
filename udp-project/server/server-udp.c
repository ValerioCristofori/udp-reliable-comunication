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
#include "defines.h"



int                        numthreads;     // Number of thread open for conns                      
pthread_mutex_t            mut          = PTHREAD_MUTEX_INITIALIZER;   //  Mutex for upgrade numthread resource
R                        **relations;		     // Ptr to the list of relations between one thread and its sender state
int                        timeout; //Timeout of the go back n protocol
int                        server_port;  //Num port where main thread wait connections
int                        window;    //the dimension of the window(packets in fly)
int                        prob_loss;


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
    
    FILE    *p;   //file stream for output of shell script
    char     command[FILENAME_LENGTH + 16];  //set of parsed parameters for shell 
    char     buffer[MAXLINE];    //result of shell script

    sprintf(command, "./script-shell/search_dir.sh %s", filename);
    p = popen(command, "r");  /* launching shell script and creating a pipe 
										for the results */
    
    if (p != NULL) {
      fscanf(p, "%[^\n]", buffer);
      if( strstr(buffer, filename) == NULL ){
          red();
          printf("file doesnt exist\n");
          reset_color();
          return -1;
      }
      if( strstr(buffer, "./root/") != NULL ){
          red();
          printf("too many file matches with this filename\n");
          reset_color();
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
                red();
                printf("The client finished through signal.Exiting from the thread child\n" );
                reset_color();
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
  
  int   lower = 1024;
  int   upper = 49151;

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


  struct  sockaddr_in     clientaddr, addr;             //structs for socket connection
  Datagram               *datagram_ptr;                 //pointer to the datagram, packet to send    
  int                     sock, sock_data, fd;          //descriptors
  int                     n, client_port, tmp, ret, size;  //int temporary variables
  socklen_t               len;                          //length of sockaddr
  FILE*                   fp;                           //file stream pointer of the datagram->file
  pthread_t               whoami = pthread_self();      //whoami
  char                    command[FILENAME_LENGTH + 16];//entire command for a shell script
  char                    path[MAXLINE];                //entire path to a file from ./root
  char                   *dirs;                         //path of the directory to create
  short                   syn;                          //short message to begin the connection
  struct timeval          timer = {0,0};            //setting the timer for exit
  
        green();
        printf("Created a thread handler\n");  
        reset_color();
        pthread_detach(whoami);    /* when a detached thread terminates releases resources */
        thread_birth();

        memcpy(&sock,sockfd,sizeof(sock));
        len = sizeof(addr);

        n = recvfrom( sock,  &syn,  sizeof(syn),  0, (struct sockaddr *)&addr,  &len );
        if( n <= 0 ){
            red();
            perror( "recvfrom error" );
            reset_color();
            thread_death();
            pthread_exit(NULL);
        } 
        green();
        printf("Syn received: %u\n", syn );
        reset_color();

    retry_num_port:  //label used when i try to use a portno that already exist

        client_port = generate_random_num_port();
        printf("%d\n", client_port );

        // create the udp socket 
        sock_data = udp_socket_init_server( &clientaddr, NULL, client_port, 0 );
        if( sock_data == -1 ){
            thread_death();
            red();
            perror("socket init error");
            reset_color();
            pthread_exit( NULL );
        }else if( sock_data == 0 ){
            yellow();
            printf("Try to use a port already used\n");
            reset_color();
            goto retry_num_port;
        }

        //send the new port number to the client
        tmp = htonl(client_port);
        n = sendto( sock ,  &tmp,  sizeof(tmp) , 0 ,
                    ( struct sockaddr *)&addr , sizeof( addr ));
        if ( n < 0 ) {
            red();
            perror ( " sendto error " );
            reset_color();
            thread_death();
            pthread_exit(NULL);
        }

        


        while(1){
            
            //malloc datagram and clear
            datagram_ptr = malloc( sizeof(*datagram_ptr) );
            memset( datagram_ptr, 0, sizeof(*datagram_ptr));

			//Wait for the request from the client
            printf("Start receiver\n");
            //start timer
            timer.tv_sec = TIMER;
            setsockopt(sock_data,SOL_SOCKET,SO_RCVTIMEO,(char*)&timer,sizeof(struct timeval));
            size = start_receiver(datagram_ptr, sock_data, &clientaddr);  // call blocker
            if( size == -1 ){
                  manage_error_signal(datagram_ptr, sock_data);
            }
            //reset the timer
            timer.tv_sec = 0;
            setsockopt(sock_data,SOL_SOCKET,SO_RCVTIMEO,(char*)&timer,sizeof(struct timeval));

            print_datagram(datagram_ptr);

            if( strcmp( datagram_ptr->command, "put") == 0 ){   //search the match with datagram->command

                        

                        //------------------- put body --------------------------
                        sprintf(path, "root/%s", datagram_ptr->filename);

                        if( strchr(datagram_ptr->filename, '/') != NULL){  //check if the program have to make some dirs

                              //take the name of all the dirs
                              //deleting the last word until '/'
                              dirs = malloc( sizeof(datagram_ptr->filename) );
                              build_directories(datagram_ptr->filename, dirs);

                              //run the shell script for making directory/ies
                              sprintf(command, "./script-shell/make_dirs.sh %s", dirs);
                              system(command);


                            
                        }
                        //creating the file with 'path' name

                        if ((fd = open( path, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
                            red();
                            printf("Error opening file %s\n", path);
                            reset_color();
                            thread_death();
                            close(sock_data);
                            pthread_exit(NULL);
                        }

                        if ((fp = fdopen(fd,"w+")) == NULL) {
                            red();
                            printf("Error fopening file %s\n",path);
                            reset_color();
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

                        //check if the file exist in the root dir and subdirs
                        //if y setup datagram
                        //if n setup error in datagram
                        ret = filename_to_path(datagram_ptr->filename, path);
                        if( ret == 1 ){
                            red();
                            printf("send error message to the client\n");
                            reset_color();
                            size = datagram_setup_error( datagram_ptr, 3 );

                        
                        }else if( ret == -1 ){
                            red();
                            printf("send error message to the client\n");
                            reset_color();
                            size = datagram_setup_error( datagram_ptr, 2 );
                            

                        }else if( ret == -2 ){
                            red();
                            printf("send error message to the client\n");
                            reset_color();
                            size = datagram_setup_error( datagram_ptr, 5 );

                        }else{
                            if( (size = datagram_setup_get( datagram_ptr, path ) ) == -1 ){  /* -1 if file doesn't exist */
                                thread_death();
                                close(sock_data);
                                pthread_exit(NULL);
                            }else{

                                printf("Content of the file '%s': %s\n", path, datagram_ptr->file );

                            }

                        }  
                        
                        printf("Start sender\n");
                        start_sender(datagram_ptr, size, sock_data, &clientaddr, whoami);


                        free(datagram_ptr);
                      




              
            }else if( strcmp(datagram_ptr->command, "list") == 0 ){

                        

                        //------------------- list body --------------------------

                        /*
                         *  launch the shell script that build a file called 'tree'
                         * 	that contains the structure of th subdirectories of ./root/
                         */
                        printf("Lauch shell script\n");
                        system("./script-shell/build_tree.sh"); // launch shell script

                        size = datagram_setup_list( datagram_ptr );


                        printf("Start sender\n");
                        start_sender(datagram_ptr, size, sock_data, &clientaddr, whoami);

                        free(datagram_ptr);

              
            }else if( strcmp(datagram_ptr->command, "exit") == 0 ){

                        //------------------- exit body --------------------------

                        green();
                        printf("Exiting from thread child...\n");
                        reset_color();
                        //close the connection and free resources
                        thread_death(); 
                        close(sock_data);
                        pthread_exit(NULL);

            }
            else{
              red();
              perror("error with the arguments passed\n");
              reset_color();
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
  
  int	                    sockfd; 						  //file descriptor
  fd_set                  sockets;              //set of file descriptors of the sockets
  struct sockaddr_in	    servaddr;             //address variable
  pthread_t               threads[MAX_THREADS]; //all the threads references 
  
      printf("\033[2J\033[H");

      if (argc != 5) { /* controlla numero degli argomenti */
          red();
          fprintf(stderr, "howtouse: ./server-udp <Server port number>  <Window size>  <Timeout dimension>  <Loss probability>\n");
          reset_color();
          exit(1);
      }

      if (parse_argv( argv ) == -1){
          red();
          fprintf(stderr, "howtouse: ./server-udp <Server port number>  <Window size>  <Timeout dimension>  <Loss probability>\n");
          reset_color();
          exit(1);
      }


      // clear the address
      memset((void *)&servaddr, 0, sizeof(servaddr));
      // create the udp socket 
      sockfd = udp_socket_init_server( &servaddr, NULL, server_port , 1 );
      if( sockfd == -1 ){
        red();
        perror("socket server init error");
        reset_color();
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
      numthreads  = 0;
      

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
                  red();
                  perror("select()");
                  reset_color();
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
