#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "defines.h"


int parse_argv( char **argv ){

  int ret;

    ret = atoi( argv[2] );   //take port number
    if ( ret == 0 ){
        printf("Port number exception\n");
        return -1;
    }
    server_port = ret;

    ret = atoi( argv[3] );   //take window size
    if ( ret == 0 ){
        printf("Window size exception\n");
        return -1;
    }
    window      = ret;

    ret = atoi( argv[4] );   //take timeout dimension
    if ( ret == 0 ){
        printf("Timeout exception\n");
        return -1;
    }
    timeout     = ret;


    //take loss probability
    if ( (prob_loss = strtod(argv[5], NULL) ) == 0.0 ){
        printf("Probability exception\n");
        return -1;
    }


    return 0;
}



int udp_socket_init_client( struct sockaddr_in*   addr,  char*   address, int   num_port ){

  /*
   *  On success return socket file descriptor
   *  else -1
   */

  int sockfd;
  int option = 1; // option to set SO REUSEADDR

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  /* create UDP socket */
      perror("errore in socket");
      return -1;
    }

    addr->sin_family = AF_INET; /* type socket family */
  addr->sin_port = htons(num_port); /* set port number */

    /* allow to bind on local used address */
  setsockopt(sockfd ,SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));

  
  //socket of a client app
  if (inet_pton(AF_INET, address, &(addr->sin_addr) ) <= 0) {
      fprintf(stderr, "errore in inet_pton per %s", address);
    return -1;
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


char** str_split(char* a_str, const char a_delim){
    
    /*
     * Return the char** contains 
     * the tokens of strings passed 
     * in a_str delimited by ' '
     * 
     * Used to parse the input of: command  {filename}
     */
    
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[3];
    delim[0] = a_delim;
    delim[1] = '\0';
    delim[2] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
