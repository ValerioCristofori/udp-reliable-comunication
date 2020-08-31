#include <assert.h>
#include <math.h>

#include "defines.h"


int parse_argv( char **argv ){

  int ret;

    ret = atoi( argv[1] );   //take port number
    if ( ret == 0 ){
        printf("Port number exception\n");
        return -1;
    }
    server_port = ret;

    ret = atoi( argv[2] );   //take window size
    if ( ret == 0 ){
        printf("Window size exception\n");
        return -1;
    }
    window      = ret;

    ret = atoi( argv[3] );   //take timeout dimension
    if ( ret == 0 ){
        printf("Timeout exception\n");
        return -1;
    }
    timeout     = ret;


    //take loss probability
    prob_loss = atoi(argv[4]);
    if ( prob_loss < 0 || prob_loss > 100 ){
        printf("Probability exception\nType number x tc. 0 < x < 100\n");
        return -1;
    }



    return 0;
}


int udp_socket_init_server( struct sockaddr_in*   addr,  char*   address, int   num_port, int option ){

	/*
	 *	On success return socket file descriptor
	 *  If bind return an errno EADDRINUSE -> return 0 to allow the exception manage in the server
	 * 	else -1
	 */

	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 	/* create UDP socket */
      red(); 
	    perror("errore in socket");
      reset_color();
	    return -1;
    }

    addr->sin_family = AF_INET; /* type socket family */
	addr->sin_port = htons(num_port); /* set port number */

    if(option){
		/* allow to bind on local used address */
    	setsockopt(sockfd ,SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
    }
	if( address != NULL ){	
		addr->sin_addr.s_addr = inet_addr(address); /* socket accept packet from 'address' only */
	}else{
		addr->sin_addr.s_addr = htonl(INADDR_ANY); /* socket accept packet from every location */
	}


	/* assign address */
	if (bind(sockfd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
      red();
	    perror("errore in bind");
      reset_color();
	    if( errno == EADDRINUSE ){
	    	return 0;
	    }
	    exit(1);
	}

	return sockfd;
	
}

int generate_random_num_port(){
  
  /*
   *  Generate a random number betw 'lower' and 'upper'
   *    This is the portno of a connection
   *    When the conn is closed the number is reused 
   *    through the exception of the binding in the socket
   */
  
  int   lower = 1024;
  int   upper = 49151;

  return ( rand() % ( upper - lower + 1 ) + lower );

}

void build_directories( char *path, char *dirs ){

	  /*
	   * This procedure set 'dirs' the entire 
	   * directories path until the file location
	   */
	  
      char ch;
      int count = 0, i = 0;    //number of '/'

      ch = path[0];

      while( ch != '\0' ){

          if( ch == '/' ) count++;
          ch = path[i++];

      } 

      
      i = 0;
      while( count ){

          ch = path[i];
          if( ch == '/' ) count--;
          
          dirs[i] = ch;
          i++;

      }
      dirs[i++] = '\0';

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
