#include <assert.h>
#include <math.h>

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
    prob_loss = atoi(argv[5]);
    if ( prob_loss < 0 || prob_loss > 100 ){
        printf("Probability exception\nType number x tc. 0 < x < 100\n");
        return -1;
    }

    ret = atoi( argv[6] );   //take timeout adaptive or not
    if( ret == 1 ){
        printf("Adaptive timeout\n");
    }else if( ret == 0 ){
        printf("Not adaptive timeout\n");
    }else{
        printf("Probability exception\n");
        return -1;
    }

    adaptive = ret;


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
      red();
      perror("errore in socket");
      reset_color();
      return -1;
    }

    addr->sin_family = AF_INET; /* type socket family */
  addr->sin_port = htons(num_port); /* set port number */

    /* allow to bind on local used address */
  setsockopt(sockfd ,SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));

  
  //socket of a client app
  if (inet_pton(AF_INET, address, &(addr->sin_addr) ) <= 0) {
      red();
      fprintf(stderr, "errore in inet_pton per %s", address);
      reset_color();
    return -1;
  }

  return sockfd;

  
}

double deviation(double * dev_RTT, double *sample_RTT, double *estimateRTT) {

    return (1-BETA)*(*dev_RTT) + BETA*abs(*sample_RTT - *estimateRTT);
}

double estimateRTT(double *estimate_RTT, double *sample_RTT) {
    
      return (((1 - ALPHA) * (*estimate_RTT)) + (ALPHA * (*sample_RTT)));
}


int change_adaptive_timer(struct timeval time_begin, struct timeval time_end, double *estimate_RTT, double *sample_RTT, double * dev_RTT){

            
        
        *sample_RTT = (double)(time_end.tv_usec - time_begin.tv_usec) / 1000000 + (double)(time_end.tv_sec - time_begin.tv_sec);
        cyan();
        printf("Calculated RTT:      %f\n", *sample_RTT );

        //calcolo nuovo timer
        *estimate_RTT = estimateRTT(estimate_RTT, sample_RTT);
        printf("Estimated RTT:       %f\n", (*estimate_RTT) );
        *dev_RTT = deviation(dev_RTT, sample_RTT, estimate_RTT);     
        printf("Estimated deviation: %f\n", *dev_RTT );
        double current_timer = *estimate_RTT + 4 * (*dev_RTT);
        printf("Change timer to      %f\n", current_timer );
        reset_color();
        
        if( (int)current_timer != 0 )return (int)current_timer;
        return 1;
}

int reset_adaptive_timer(double *estimate_RTT, double *sample_RTT, double * dev_RTT){


          //ripristino timer
          *dev_RTT = 0;
          *estimate_RTT = timeout;
          *sample_RTT = 0;
              
          cyan();
          printf("Reset adaptive timer to %d\n", timeout );
          reset_color();
          return timeout;

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
