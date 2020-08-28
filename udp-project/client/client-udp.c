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

#include "defines.h"


char *gets(char *s);


struct sockaddr_in       servaddr;
int                      sockfd;              // file descriptor
Datagram                *datagram_ptr;        //pointer to the packet struct to be sent/received
int                      timeout; //Timeout of the go back n protocol
int                      server_port;  //Num port where main thread wait connections
int                      window;    //the dimension of the window(packets in fly)
int                      prob_loss;
int                      adaptive;



void handler_alarm_conn (int ign)  /* handler for SIGALRM */
{
	/*
	 * Exit from the client --> server not running
	 */
    red();
    printf("Error with the connection: server doesn't respond\n");
    reset_color();
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

//






int main(int argc, char *argv[]) {
  int                      maxfd;								  //number of byte receved from recvfrom or sendto
  int                      n, tmp, size;          //temporary variables
  fd_set                   fds;                   //set of descriptors
  char                     buff[MAXLINE];         //input string of the user
  char                    *ptr;                   //temporary pointer for user input management
  socklen_t                len;                   //length of the sockaddr struct
  char                   **arguments = NULL;      //arguments passed from input and parsed
  FILE*                    fp;                    //file pointer 
  char                    *dirs;                  //path of directories to create
  char                     command[FILENAME_LENGTH + 16];
  int                      fd;
  short                    syn = 0x10;            //syn message to begin the connection
  int                      client_port;           //negotiated port with server
  struct sigaction         sa;                    //sigaction struct to manage signal exceptions


  

  printf("\033[2J\033[H");


  if (argc != 7) { /* controlla numero degli argomenti */
      red();
      fprintf(stderr, "howtouse: ./client-udp <indirizzo IP server>  <port number>  <window size>  <timeout dimension>  <loss probability> <Adaptive timeout mode 1=on/0=off>\n");
      reset_color();
      exit(1 );
  }

  if ( parse_argv( argv ) == -1 ){
      red();
      fprintf(stderr, "howtouse: ./client-udp <indirizzo IP server>  <port number>  <window size>  <timeout dimension>  <loss probability> <Adaptive timeout mode 1=on/0=off>\n");
      reset_color();
      exit(1);
  }

  /* handler a sigalarm for bad connection */
  sa.sa_handler = handler_alarm_conn;
  if (sigfillset (&sa.sa_mask) < 0){ /* block everything in handler */
      red();
      perror("sigfillset() failed");
      reset_color();
      exit(0);
  }
  sa.sa_flags = 0;

    if (sigaction (SIGALRM, &sa, 0) < 0){
      red();
      perror("sigaction() failed for SIGALRM");
      reset_color();
      exit(0);
  }

  alarm(THRESHOLD); //alarm signal if server dont respond
					//until threshold sec 

  
  memset((void *)&servaddr, 0, sizeof(servaddr));      /* set 0 servaddr struct */
  
  sockfd = udp_socket_init_client( &servaddr, argv[1], server_port );
  if( sockfd == -1 ){
    red();
    perror("socket init error");
    reset_color();
    exit(-1);
  }


  blue();
  printf("Connection to the server on address %s and port %d\n", argv[1], server_port );
  reset_color();

  FD_ZERO(&fds);  /* Init with 0 all the entry of fds set */

  //send del pacchetto di syn per richiedere una porta
  yellow();
  printf("Syn sent\n");
  reset_color();

  n = sendto( sockfd ,  &syn,  sizeof(syn) , 0 ,
              ( struct sockaddr *)&servaddr , sizeof( servaddr ));
  if ( n < 0 ) {
    red();
    perror ( " sendto error " );
    reset_color();
    exit ( -1);
  }
  len = sizeof(servaddr);
  //Wait the new port no 
  n = recvfrom( sockfd ,  &tmp,  sizeof(tmp),  0, (struct sockaddr *)&servaddr,  &len );
  if( n <= 0 ){
      red();
      perror( "recvfrom error" );
      reset_color();
      exit(-1);
  }
  client_port = ntohl(tmp); //setting the new port number for new socket 
  
  alarm(0);   // reset alarm --> good connection

  //change server port with the negotiate one
  memset((void *)&servaddr, 0, sizeof(servaddr));
  
  sockfd = udp_socket_init_client( &servaddr, argv[1], client_port );
  if( sockfd == -1 ){
    red();
    perror("socket init error");
    reset_color();
    exit(-1);
  }
  yellow();
  printf("\nNew socket with %d port number\n", ntohs(servaddr.sin_port) );
  reset_color();

  //set the handler for sigint signal
  sa.sa_handler = handler_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    red();
      printf("sigaction error\n");
      exit(-1); 
    reset_color();
  }


  while(1){
    
    //-----------------------   Client body

          green();
          printf("\n\nType something:\n");
          reset_color();

          /* use fileno() for conversion from FILE* to int */
          FD_SET( fileno(stdin), &fds );  /* add stdin file descriptor */
          FD_SET( sockfd, &fds );           /* add socket with server descriptor */
          

          maxfd = (fileno(stdin) < sockfd) ? (sockfd + 1): (fileno(stdin) + 1);  /* maxfd begin the max file descriptor + 1 */

          if (select(maxfd, &fds, NULL, NULL, NULL) < 0 ) { /* wait a descriptor ready in reading */
                red();
                perror("select error");
                reset_color();
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
              arguments = str_split(buff, ' ');/* parsing the shell command */
     
              if( arguments == NULL ){
                  cyan();
                  printf("\n\nWrong arguments\nTry to type:\nput <path/filename>\nget <path/filename>\nlist\nexit\n");
                  reset_color();
                  goto retry;
              }
              

              //malloc datagram and clear
              datagram_ptr = malloc( sizeof(*datagram_ptr ));
              memset( datagram_ptr, 0, sizeof(*datagram_ptr));



              if( strcmp(arguments[0], "put") == 0 ){

                              


                              //------------------- put body --------------------------
                              


                              if( arguments[1] == NULL ){
                                red();
                                  perror("You must insert the name of the file.");
                                reset_color();
                                  goto retry;
                              }
                              yellow();
                              printf("Put\n" );
                              reset_color();

                              

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
                              //start the put test if its defined
                              #ifdef PUT_TEST
                                  struct timeval test_timer_begin = {0,0};
                                  gettimeofday(&test_timer_begin, NULL);
                              #endif    
                              
                              start_sender(datagram_ptr, size, sockfd, &servaddr); /* Send the request to the server */

                              #ifdef PUT_TEST
                                  struct timeval test_timer_end = {0,0};
                                  gettimeofday(&test_timer_end, NULL);
                                  cyan();
                                  printf("DATAGRAM SEND IN %f SECONDS\n", (double)((double)(test_timer_end.tv_usec - test_timer_begin.tv_usec) / 1000000 + (double)(test_timer_end.tv_sec - test_timer_begin.tv_sec)) );
                                  reset_color();
                              #endif

              }else if( strcmp(arguments[0], "get") == 0 ){
                

                              //------------------- get body --------------------------

                              if( arguments[1] == NULL ){
                                  red();
                                  perror("You must insert the name of the file.");
                                  reset_color();
                                  goto retry;
                              }

                              yellow();
                              printf("Get\n" );
                              reset_color();

                              size = datagram_setup_get( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr); /* Send the request to the server */

                              //clear datagram
                              memset( datagram_ptr, 0, sizeof(*datagram_ptr));

                              printf("Start receiver\n");
                              size = start_receiver(datagram_ptr, sockfd, &servaddr); /* Wait the response from the server */
                              if(size == -1){
                                    print_error_datagram(datagram_ptr, sockfd);
                                    goto retry;
                              }
                                                          
                              /*
                               *  Parsing the datagram and
                               *  open a file with filename datagram->filename
                               */

                              if( strchr(datagram_ptr->filename, '/') != NULL){  //check if the program have to make some dirs

                                    //take the name of all the dirs
                                    //deleting the last chars until '/'
                                    dirs = malloc( sizeof(datagram_ptr->filename) );
                                    build_directories(datagram_ptr->filename, dirs);

                                    //run the shell script for making directory/ies
                                    sprintf(command, "./script-shell/make_dirs.sh %s", dirs);
                                    system(command);


                            
                              }

                              printf("Filename: %s\n", datagram_ptr->filename);
                          

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

                              green();
                              printf("Success on download file %s\n", datagram_ptr->filename );
                              reset_color();







                
              }else if( strcmp(arguments[0], "list") == 0 ){

                


                              //------------------- list body --------------------------


                              yellow();
                              printf("List\n" );
                              reset_color();

                              size = datagram_setup_list( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr);  /* Send the request to the server */


                              //clear datagram
                              //---------------------------------------
                              //free(datagram_ptr);

                              printf("Start receiver\n");
                              size = start_receiver(datagram_ptr, sockfd, &servaddr);   /* Wait the response from the server */
                              if(size == -1){
                                    print_error_datagram(datagram_ptr, sockfd);
                                    goto retry;
                              }
                              print_datagram(datagram_ptr);


                              printf("List of the files: %s\n", datagram_ptr->file );  //print the tree of the files
                              
                       
                
              }else if( strcmp(arguments[0], "exit") == 0 ){

                              red();
                              printf("Exiting...\n"); 
                              reset_color();
                              

                              size = datagram_setup_exit( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr); /* Send exit message to the server */

                              free(datagram_ptr);
                              free(arguments);           
                              break;


              }else{
                  red();
                  printf("\nWrong arguments\nTry to type:\nput <path/filename>\nget <path/filename>\nlist\nexit\n");
                  reset_color();
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
