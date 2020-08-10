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



ssize_t FullWrite ( int fd , const void * buf , size_t count ){

	size_t nleft;
	size_t nwritten;

	nleft = count;
	while( nleft < 0 ){
		
		if( (nwritten = write( fd, buf, nleft ) ) < 0){
			if ( errno == EINTR ){
				continue;
			}else{
				return nwritten;
			}
		}

		nleft -= nwritten;
		buf += nwritten;
	}
	return nleft;
}


int udp_socket_init_client( struct sockaddr_in*   addr,  char*   address, int   num_port ){

	/*
	 *	On success return socket file desriptor
	 *  else -1
	 */

	int sockfd;
	int option = 1;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 	/* create UDP socket */
	    perror("errore in socket");
	    return -1;
    }

    addr->sin_family = AF_INET; /* tipo di socket */
	addr->sin_port = htons(num_port); /* numero di porta del server */

    /* permette di eseguire bind su indirizzi locali che sono gia in uso da altri socket */
	setsockopt(sockfd ,SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));

	
	//socket of a client app
	/* assegna l'indirizzo del server . L'indirizzo e una stringa da convertire in intero secondo network byte order. */
	if (inet_pton(AF_INET, address, &(addr->sin_addr) ) <= 0) {
	    fprintf(stderr, "errore in inet_pton per %s", address);
		return -1;
	}

	return sockfd;

	
}


int udp_socket_init_server( struct sockaddr_in*   addr,  char*   address, int   num_port, int option ){

	/*
	 *	On success return socket file desriptor
	 *  else -1
	 */

	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 	/* create UDP socket */
	    perror("errore in socket");
	    return -1;
    }

    addr->sin_family = AF_INET; /* tipo di socket */
	addr->sin_port = htons(num_port); /* numero di porta del server */

    if(option){
        /* permette di eseguire bind su indirizzi locali che sono gia in uso da altri socket */
    	setsockopt(sockfd ,SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
    }
	if( address != NULL ){	
		addr->sin_addr.s_addr = inet_addr(address); /* il socket accetta pacchetti da l'indirizzo address */
	}else{
		addr->sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
	}


	/* assegna l'indirizzo al socket */
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


