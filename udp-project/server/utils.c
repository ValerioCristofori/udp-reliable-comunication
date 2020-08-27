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
	    perror("errore in socket");
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
	    perror("errore in bind");
	    if( errno == EADDRINUSE ){
	    	return 0;
	    }
	    exit(1);
	}

	return sockfd;
	
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



