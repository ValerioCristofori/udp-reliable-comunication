/*
 * 		UDP Client
 * 		This client provide 3 command:
 * 			1- get command: provide the download of a specific 
 * 				file in the directory's tree.
 * 			2- put command: provide the upload of a specific 
 * 				file in the directory's tree.
 * 			3- list command: provide the list of the directory's tree.		
 *		All this features are allowed through reliable data transfer.  
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
#define MAXFILE           4096
#define FILENAME_LENGHT   32
#define cipherKey         'S'


char *gets(char *s);

typedef struct datagram_value {
      
      char command[5];
      char filename[FILENAME_LENGHT];
      char file[MAXFILE];
      char error_message[32];
      int  die_sig;

} Datagram;

struct sockaddr_in       servaddr;
int                      sockfd;              // file descriptor
Datagram                 datagram;


void handler() {
    
    int       n;    
       

        //setup exit message in datagram.error_message
        datagram.die_sig = 1; 

        //inviare exit al server su un campo particolare
        n = sendto( sockfd ,  &datagram,  sizeof(datagram) , 0 ,
                    ( struct sockaddr *)&servaddr , sizeof( servaddr ));
        if ( n < 0 ) 
 {         perror ( " sendto error " );
          exit ( -1);
        }

        close(sockfd);

}

int recv_datagram(){



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

void decrypt_string(char* dcstring, char* str ){

      int     i;
      char    ch;

      for (i = 0; i < MAXFILE; i++) { 
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


void path_to_filename( char* path , char* filename ){

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
        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
        strcpy( datagram_ptr->filename, arguments[1] );

        // build the attr datagram->file with the file stream
 
        for ( int i = 0; i < MAXFILE; i++) { 
            char ch = fgetc(fp); 
            char ch2 = Cipher(ch); 
            datagram_ptr->file[i] = ch2; 
            if (ch == EOF) 
                return 1; 
        } 
        return 0; 

}


void datagram_setup_get( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
        strcpy( datagram_ptr->filename, arguments[1] );
 
        

}


void datagram_setup_list( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
 
        

}








int main(int argc, char *argv[]) {
  int                      maxfd;								// number of byte receved from recvfrom or sendto
  int                      n, tmp;
  fd_set                   fds;                 // set di descrittori
  char                     buff[MAXLINE];        // input string of the user
  socklen_t                len;
  char**                   arguments = NULL;
  int                      num_arguments;
  FILE*                    fp;
  int                      fd;
  char                     decrypted_string[MAXFILE];
  char*                    command;
  short                    syn = 0x10;
  int                      client_port;
  struct sigaction         sa;


  




  if (argc != 2) { /* controlla numero degli argomenti */
    fprintf(stderr, "howtouse: ./client <indirizzo IP server>\n");
    exit(1);
  }

  
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
          printf("Type something...\n");

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
              arguments = NULL;

              //if (fgets(buff, MAXLINE, stdin) == NULL)
               //   break; /* non vi sono dati */   -------------------------- provare a mettere fgets e fullwrite

              scanf("%[^\n]s", buff);
              printf("il buffer contiene: %s\n",  buff);
              num_arguments = split(buff, ' ', &arguments);/* parsing the shell command */
              while( (getchar()) != '\n');
              if( arguments == NULL ){
                  printf("Wrong arguments\nTry to type:\n-put\n-get\n-list\n-exit\n");
                  continue;
              }
              command = arguments[0];
              printf("%s\n", command );

              bzero( &datagram, sizeof(datagram) );


              if( stringCmpi(command, "put") == 0 ){

                              


                              //------------------- put body --------------------------
                              



                              if( arguments[1] == NULL ){
                                  perror("You must insert the name of the file.");
                                  break;
                              }

                              printf("put\n" );

                              fp = fopen( arguments[1], "r"); 
                              printf("\nFile Name Received: %s\n", arguments[1]); 
                              if (fp == NULL) 
                                  printf("\nFile open failed!\n"); 
                              else
                                  printf("\nFile Successfully opened!\n"); 

                              datagram_setup_put(  &datagram,  arguments,  fp  );

                              //try to send the datagram to the server
                              n = sendto( sockfd ,  &datagram,  sizeof(datagram) , 0 ,
                                          ( struct sockaddr *)&servaddr , sizeof( servaddr ));
                              if ( n < 0 ) {
                                perror ( " sendto error " );
                                exit ( -1);
                              }


                              //clear datagram
                              bzero( &datagram, sizeof(datagram) );


                              //wait for the response
                              //error or success message

                              


            


              }else if( stringCmpi(command, "get") == 0 ){
                
                



                              //------------------- get body --------------------------

                              if( arguments[1] == NULL ){
                                  perror("You must insert the name of the file.");
                                  break;
                              }

                              printf("get\n");

                              datagram_setup_get( &datagram,  arguments );

                              //try to send the datagram to the server
                              n = sendto( sockfd ,  &datagram,  sizeof(datagram) , 0 ,
                                          ( struct sockaddr *)&servaddr , sizeof( servaddr ));
                              if ( n < 0 ) {
                                perror ( " sendto error " );
                                exit ( -1);
                              }

                              //clear datagram
                              bzero( &datagram, sizeof(datagram) );

                              len = sizeof(servaddr);
                              //aspetta la risposta del server
                              n = recvfrom( sockfd ,  &datagram,  sizeof(datagram),  0, (struct sockaddr *)&servaddr,  &len );
                              if( n <= 0 ){
                                  perror( "recvfrom error" );
                                  exit(-1);
                              }

                              //datagram ricevuto
                              

                              //controllo che non ci siano stati errori
                              //---------------------------------------------------- da implementare

                                
                              /*
                               *  voglio parsare il datagram e 
                               *  creare un file fscanf il contenuto nel buffer
                               */
                              decrypt_string( decrypted_string, datagram.file );
                              printf("Decrypted content: %s\n", decrypted_string );

                              printf("filename: %s\n", datagram.filename);
                          

                              if ((fd = open( datagram.filename, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
                                  printf("Error opening file %s\n", datagram.filename);
                                  exit(-1);
                              }

                              if ((fp = fdopen(fd,"w+")) == NULL) {
                                  printf("Error fopening file %s\n", datagram.filename);
                                  exit(-1);
                              }

                              if ( fprintf( fp, "%s", decrypted_string ) < 0 ){
                                  perror ( " fprintf error " );
                                  exit(-1);
                              }
                              fflush(fp);

                              printf("Success on download file %s\n", datagram.filename );











                
              }else if( stringCmpi(command, "list") == 0 ){

                


                              //------------------- list body --------------------------


                              printf("list\n" );

                              datagram_setup_list( &datagram,  arguments );


                              //try to send the datagram to the server
                              n = sendto( sockfd ,  &datagram,  sizeof(datagram) , 0 ,
                                          ( struct sockaddr *)&servaddr , sizeof( servaddr ));
                              if ( n < 0 ) {
                                perror ( " sendto error " );
                                exit ( -1);
                              }

                              //clear datagram
                              bzero( &datagram, sizeof(datagram) );


                              len = sizeof(servaddr);
                              //aspetta la risposta del server
                              n = recvfrom( sockfd ,  &datagram,  sizeof(datagram),  0, (struct sockaddr *)&servaddr,  &len );
                              if( n <= 0 ){
                                  perror( "recvfrom error" );
                                  exit(-1);
                              }

                              printf("List of the files:\n%s\n", datagram.file ); // printare diveramente
                              // attenzione all'EOF quando viene printato , non bello da vedere ------------------------------------




                
              }else if( stringCmpi(command, "exit") == 0 ){

                              printf("Exiting...\n");            
                              break;


              }else{
                 
                              printf("Wrong arguments\nTry to type:\n-put\n-get\n-list\n-exit\n");
                              perror("error with the arguments passed\n");
                              continue;
              }

          }



  }

  
  
  // normal exit
  exit(0);
}
