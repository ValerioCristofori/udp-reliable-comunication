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
#define MAXLINE           1024
#define THRESHOLD         3

#define cipherKey         'S'


char *gets(char *s);


struct sockaddr_in       servaddr;
int                      sockfd;              // file descriptor
Datagram                 datagram;
Datagram                 *datagram_ptr;


void handler_alarm_conn (int ign)  /* handler for SIGALRM */
{
    printf("Error with the connection: server doesn't respond\n");
    exit(0);
}

void handler() {
    
    int       n;    
       

        //setup exit message in datagram.error_message
        //datagram.die_sig = 1; 

        //inviare exit al server su un campo particolare
        n = sendto( sockfd ,  &datagram,  sizeof(datagram) , 0 ,
                    ( struct sockaddr *)&servaddr , sizeof( servaddr ));
        if ( n < 0 ) 
 {         perror ( " sendto error " );
          exit ( -1);
        }

        close(sockfd);

}

void print_datagram( Datagram *datagram_ptr){

      printf("\nCommand: %s, sizeof(): %ld\n", datagram_ptr->command, sizeof(datagram_ptr->command) );
      printf("Filename: %s, sizeof(): %ld\n", datagram_ptr->filename, sizeof(datagram_ptr->filename) );
      printf("Filename size: %d, sizeof(): %ld\n", datagram_ptr->length_filename , sizeof(datagram_ptr->length_filename));
      printf("Datagram size: %d, sizeof(): %ld\n", datagram_ptr->datagram_size, sizeof(datagram_ptr->datagram_size) );
      printf("File size: %d, sizeof(): %ld\n", datagram_ptr->length_file, sizeof(datagram_ptr->length_file) );
      printf("File content: %s, sizeof(): %ld\n", datagram_ptr->file, sizeof(datagram_ptr->file) );
      printf("Error message: %s, sizeof(): %ld\n", datagram_ptr->error_message, sizeof(datagram_ptr->error_message) );
      printf("Code error: %d, sizeof(): %ld\n", datagram_ptr->err, sizeof(datagram_ptr->err) );
}

void print_file( char *file, int length ){

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



// function to clear datagram 
void clear_datagram(Datagram* datagram){

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


// function to encrypt/decrypt 
char Cipher(char ch) 
{ 
    return ch ^ cipherKey; 
}

void decrypt_string(char* dcstring, char* str, int length ){

      int     i;
      char    ch;

      for (i = 0; i < length; i++) { 
        ch = str[i]; 
        ch = Cipher(ch); 
        if (ch == EOF){ 
            return;
        } 
        else{
            dcstring[i] = ch;
        }
      }

}


void path_to_filename( char *path , char *filename ){

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



int datagram_setup_put( Datagram* datagram_ptr, char** arguments,  FILE* fp ){
        
        int length = 0, i;
        char ch, ch2;


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );

        strcpy( datagram_ptr->filename, arguments[1] );

        datagram_ptr->length_filename = strlen(arguments[1]);

        // build the attr datagram->file with the file stream
        while( (ch = fgetc(fp)) != EOF ){
            length++;
        }
        datagram_ptr->length_file = length + 1;

        rewind(fp);

        for ( i = 0; i < length + 1; i++) {  // +1 for the '/0' char

            ch = fgetc(fp);  
            ch2 = Cipher(ch); 
            datagram_ptr->file[i] = ch2; 
        }

        datagram_ptr->file[i] = EOF;
        printf("File dimension %d\n", datagram_ptr->length_file);
        datagram_ptr->datagram_size = sizeof(*datagram_ptr);

        print_datagram(datagram_ptr);
        return datagram_ptr->datagram_size;

}


int datagram_setup_get( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
        
        strcpy( datagram_ptr->filename, arguments[1] );

        datagram_ptr->length_filename = strlen(arguments[1]);
        printf("Length filename: %d\n", datagram_ptr->length_filename );
        datagram_ptr->length_file = 0;

        datagram_ptr->datagram_size = sizeof(*datagram_ptr);

        print_datagram(datagram_ptr);
        
        return datagram_ptr->datagram_size;
        

}


int datagram_setup_list( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );

        datagram_ptr->length_file = 0;
        datagram_ptr->datagram_size = sizeof(*datagram_ptr);

        print_datagram(datagram_ptr);


        return datagram_ptr->datagram_size;
 
        

}

int datagram_setup_exit( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );

        datagram_ptr->length_file = 0;
        datagram_ptr->datagram_size = sizeof(*datagram_ptr);

        print_datagram(datagram_ptr);


        return datagram_ptr->datagram_size;
 
        

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
  int                      fd;
  char                     decrypted_string[MAXFILE];
  short                    syn = 0x10;
  int                      client_port;
  struct sigaction         sa;


  




  if (argc != 2) { /* controlla numero degli argomenti */
    fprintf(stderr, "howtouse: ./client <indirizzo IP server>\n");
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

  alarm(THRESHOLD);

  
  memset((void *)&servaddr, 0, sizeof(servaddr));      /* azzera servaddr */
  
  sockfd = udp_socket_init_client( &servaddr, argv[1], SERV_PORT );
  if( sockfd == -1 ){
    perror("socket init error");
    exit(-1);
  }



  printf("Connection to the server on address %s and port %d\n", argv[1], SERV_PORT );


  FD_ZERO(&fds);  /* inizializza a 0 il set dei descrittori in lettura */

  //send del pacchetto di syn per richiedere una porta
  printf("Syn sent\n");

  n = sendto( sockfd ,  &syn,  sizeof(syn) , 0 ,
              ( struct sockaddr *)&servaddr , sizeof( servaddr ));
  if ( n < 0 ) {
    perror ( " sendto error " );
    exit ( -1);
  }
  len = sizeof(servaddr);
  n = recvfrom( sockfd ,  &tmp,  sizeof(tmp),  0, (struct sockaddr *)&servaddr,  &len );
  if( n <= 0 ){
      perror( "recvfrom error" );
      exit(-1);
  }
  client_port = ntohl(tmp);
  
  alarm(0);

  //change server port with the negotiate one
  memset((void *)&servaddr, 0, sizeof(servaddr));      /* azzera servaddr */
  
  sockfd = udp_socket_init_client( &servaddr, argv[1], client_port );
  if( sockfd == -1 ){
    perror("socket init error");
    exit(-1);
  }

  printf("Ricevuto nuovo socket con porta %d\n", ntohs(servaddr.sin_port) );

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
      printf("sigaction error\n");
      exit(-1); 
  }


  while(1){
    
    //-----------------------   Client body
          printf("\nType something:\n");

          /* uso fileno() per la conversione FILE* in int */
          FD_SET( fileno(stdin), &fds );  /* aggiungo il descrittore dello std input */
          FD_SET( sockfd, &fds );           /* aggiungo il descrittore del socket con il server */
          

          maxfd = (fileno(stdin) < sockfd) ? (sockfd + 1): (fileno(stdin) + 1);  /* maxfd il massimo fd + 1 */

          if (select(maxfd, &fds, NULL, NULL, NULL) < 0 ) { /* attendo descrittore pronto in lettura */
                perror("select error");
                exit(1);
          }

          /*
           *  al ritorno dell a chiamata select servo il descrittore 
           *  che ha causato l'uscita
           */

          

          /* controllo se lo stdin è leggibile */
          if (FD_ISSET(fileno(stdin), &fds)) {   /*  controllo se lo stdin è nel set dei file descriptors */ 
              ;
              

               
              memset(&buff, 0, sizeof(buff));

              if (fgets(buff, sizeof(buff), stdin) != NULL)
              {
                if ((ptr = strchr(buff, '\n')) != NULL)
                  *ptr = '\0';
                
              }
              printf("il buffer contiene: %s\n",  buff);
              arguments = str_split(buff, ' ');/* parsing the shell command */
     
              if( arguments == NULL ){
                  printf("Wrong arguments\nTry to type:\n-put\n-get\n-list\n-exit\n");
                  goto retry;
              }
              

              //malloc datagram and clear
              datagram_ptr = malloc( sizeof(*datagram_ptr) );
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

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr);



                              //wait for the response
                              //error or success message

                              


            


              }else if( strcmp(arguments[0], "get") == 0 ){
                
                



                              //------------------- get body --------------------------

                              if( arguments[1] == NULL ){
                                  perror("You must insert the name of the file.");
                                  goto retry;
                              }

                              printf("get\n");

                              size = datagram_setup_get( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr);

                              //clear datagram
                              memset( datagram_ptr, 0, sizeof(*datagram_ptr));

                              printf("Start receiver\n");
                              size = start_receiver(datagram_ptr, sockfd, &servaddr, 0.1);


                              //datagram ricevuto
                              printf("Datagram ricevuto (comando get)\n");

                              //controllo che non ci siano stati errori
                              if( datagram.err == 1 ){
                                  printf("%s\n", datagram_ptr->error_message );
                                  continue;
                              }
                                                          
                              /*
                               *  Parsing the datagram and
                               *  open a file with filename datagram.filename
                               */



                              //------------------------------------ manage input valerio/prova2




                              decrypt_string( decrypted_string, datagram_ptr->file, datagram_ptr->length_file );
                              printf("Decrypted content: %s\n", decrypted_string );

                              printf("filename: %s\n", datagram_ptr->filename);
                          

                              if ((fd = open( datagram_ptr->filename, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
                                  printf("Error opening file %s\n", datagram_ptr->filename);
                                  exit(-1);
                              }

                              if ((fp = fdopen(fd,"w+")) == NULL) {
                                  printf("Error fopening file %s\n", datagram_ptr->filename);
                                  exit(-1);
                              }

                              if ( fprintf( fp, "%s", decrypted_string ) < 0 ){
                                  perror ( " fprintf error " );
                                  exit(-1);
                              }
                              fflush(fp);

                              printf("Success on download file %s\n", datagram_ptr->filename );








                
              }else if( strcmp(arguments[0], "list") == 0 ){

                


                              //------------------- list body --------------------------


                              printf("list\n" );

                              size = datagram_setup_list( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr);


                              //clear datagram
                              //---------------------------------------
                              //free(datagram_ptr);

                              printf("Start receiver\n");
                              size = start_receiver(datagram_ptr, sockfd, &servaddr, 0.1);
                              printf("Bytes received %d\n", size );
                              print_datagram(datagram_ptr);


                              printf("List of the files: %s\n", datagram_ptr->file );
                              
                              // printf("lunghezza %d\n", datagram_ptr->length_file );
                              // print_file(datagram_ptr->file, datagram_ptr->length_file);
                              // attenzione all'EOF quando viene printato , non bello da vedere ------------------------------------




                
              }else if( strcmp(arguments[0], "exit") == 0 ){

                              printf("Exiting...\n"); 
                              

                              size = datagram_setup_exit( datagram_ptr,  arguments );

                              printf("Start sender\n");
                              start_sender(datagram_ptr, size, sockfd, &servaddr);



                              free(datagram_ptr);
                              free(arguments);           
                              break;


              }else{
                 
                              printf("Wrong arguments\nTry to type:\n-put\n-get\n-list\n-exit\n");
                              perror("error with the arguments passed\n");
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
