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
#define MAX_THREADS        10
#define cipherKey          'S'


int                        numthreads = 0;
pthread_mutex_t            mut = PTHREAD_MUTEX_INITIALIZER;




int stringCmpi(char *s1,char *s2)
{
    int i=0;
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


int datagram_setup_get( Datagram* datagram_ptr, char* filename ){
   
   FILE*      fp;


          fp = fopen( filename, "r"); 
          printf("\nFile Name Received: %s\n", filename); 
          if (fp == NULL){ 
              printf("\nFile open failed!\n"); 
              return -1;
          }
          else
              printf("\nFile Successfully opened!\n");

          // costruisco il file del datagram attraverso il file stream
 
          for ( int i = 0; i < MAXFILE; i++) { 
              char ch = fgetc(fp); 
              char ch2 = Cipher(ch); 
              datagram_ptr->file[i] = ch2; 
              if (ch == EOF) 
                  return 1; 
          } 
          return 0;
}


int datagram_setup_list( Datagram* datagram_ptr ){

    FILE* fp;
          printf("Open file that contains tree\n");
          fp = fopen( "tree", "r");  // prova ad aprire il file contenente l'albero
          if (fp == NULL) 
              printf("\nFile open failed!\n"); 
          else
              printf("\nFile Successfully opened!\n");

          for ( int i = 0; i < MAXFILE; i++) { 
              char ch = fgetc(fp);  
              datagram_ptr->file[i] = ch; 
              if (ch == EOF) 
                  return 1; 
          }

          return 0;

}


int filename_to_path( char* filename, char* path ){
    FILE *p;
    char command[FILENAME_LENGHT + 16];
    char buffer[MAXLINE];

    sprintf(command, "./search_dir.sh %s", filename);
    printf("%s is the command\n", command );

    p = popen(command, "r");
    
    if (p != NULL) {
      fscanf(p, "%s", buffer);
      if( strstr(buffer, filename) == NULL ){
          printf("file doesnt exist\n");
          return -1;
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



void setup_error_message( Datagram* datagram_ptr ){
    datagram_ptr->err = 1;
    strcpy( datagram_ptr->error_message, "Error: file not present ");
}


void thread_death(){
    pthread_mutex_lock(&mut);
    numthreads--;
    pthread_mutex_unlock(&mut);
}

void thread_birth(){
    pthread_mutex_lock(&mut);
    numthreads++;
    pthread_mutex_unlock(&mut);  
}




int generate_random_num_port(){
  int lower = 2224;
  int upper = 5000;

  srand( (unsigned int)time(NULL) ); //special seeds; only one for process

  return ( rand() % ( upper - lower + 1 ) + lower );

}


void print_buff_datagram( Datagram* datagram_ptr ){
      printf("command : %s, filename: %s\n", datagram_ptr->command, datagram_ptr->filename );
}




void *client_request( void *sockfd ){

  struct  sockaddr_in     clientaddr, relation;
  Datagram                datagram;
  int                     n, sock, sock_data, fd;
  int                     client_port, tmp, ret;
  socklen_t               len;
  FILE*                   fp;
  pthread_t               whoami = pthread_self();
  char                    decrypted_string[MAXFILE];
  char                    path_file[40];
  char                    path[MAXLINE];
  short                   syn;

        printf("Created a thread handler\n");  
        /* instead call the pthread_join func */
        pthread_detach(whoami);    /* when a detached thread terminates releases his resources */

        thread_birth();

        memcpy(&sock,sockfd,sizeof(sock));

        /*  setupping the struct for recv data */
        bzero( &datagram, sizeof(datagram) );
        len = sizeof(relation);

        n = recvfrom( sock,  &syn,  sizeof(syn),  0, (struct sockaddr *)&relation,  &len );
        if( n <= 0 ){
            perror( "recvfrom error" );
            thread_death();
            pthread_exit(NULL);
        } 
        printf("Syn received: %u\n", syn );

        print_buff_datagram(&datagram);
        client_port = generate_random_num_port();
        printf("%d\n", client_port );

        //try to send the new port number to the client
        tmp = htonl(client_port);
        n = sendto( sock ,  &tmp,  sizeof(tmp) , 0 ,
                    ( struct sockaddr *)&relation , sizeof( relation ));
        if ( n < 0 ) {
            perror ( " sendto error " );
            thread_death();
            pthread_exit(NULL);
        }



        // create the udp socket 
        sock_data = udp_socket_init_server( &clientaddr, NULL, client_port );
        if( sock_data == -1 ){
            thread_death();
            perror("socket init error");
            pthread_exit( NULL );
        }
        


        while(1){
            
            /*  setupping the struct for recv data */
            //bzero( &datagram, sizeof(datagram) );
            printf("Start receiver\n");
            start_receiver(&datagram, sock_data, &clientaddr, 0.1);
            /*n = recvfrom( sock_data,  &datagram,  sizeof(datagram),  0, (struct sockaddr *)&clientaddr,  &len );
            if( n <= 0 ){
                perror( "recvfrom error" );
                close(sock_data);
                thread_death();
                pthread_exit(NULL);
            }*/

            if( datagram.die_sig == 1 ){
                printf("The client finished through signal CTRL+C\nExiting from the thread child\n" );
                thread_death();
                close(sock_data);
                pthread_exit(NULL);
            }

            if( stringCmpi( datagram.command, "put") == 0 ){

                        

                        //------------------- put body --------------------------
                        

                        printf("put\n" );

                        decrypt_string( decrypted_string, datagram.file );
                        printf("%s\n", decrypted_string );
                        sprintf(path_file, "root/%s", datagram.filename);
                        

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

                        if ( fprintf( fp, "%s", decrypted_string ) < 0 ){
                            perror ( " fprintf error " );
                            thread_death();
                            close(sock_data);
                            pthread_exit(NULL);
                        }
                        fflush(fp);

                        printf("Success on download file %s\n", datagram.filename );


                        




            }else if( stringCmpi(datagram.command, "get") == 0 ){
              
                        

                        //------------------- get body --------------------------


                        printf("get\n");

                        /*  --------------------------------------------------------------------------------
                         *  cambiare il path del file ---------> trovare il file nell'albero e aggiungere dir padri
                         */


                        //controllo che il file sia presente nell'albero delle directories se si faccio il setup del datagram
                        ret = filename_to_path(datagram.filename, path);
                        if( ret == 1 ){
                            setup_error_message( &datagram );
                            thread_death();
                            close(sock_data);
                            pthread_exit(NULL);
                        }else if( ret == -1 ){
                            printf("send error message to the client\n");
                            setup_error_message(&datagram);
                        }else{
                            printf("path file: %s\n", path );
                            if( datagram_setup_get( &datagram, path ) == -1 ){  /* -1 if file doesn't exist */
                                setup_error_message( &datagram );
                                thread_death();
                                close(sock_data);
                                pthread_exit(NULL);
                            }

                            printf("Il contenuto del file '%s': %s\n", path, datagram.file );

                        }  
                        //try to send the datagram to the client
                        
                        /*n = sendto( sock_data ,  &datagram,  sizeof(datagram) , 0 ,
                                    ( struct sockaddr *)&clientaddr , sizeof( clientaddr ));
                        if ( n < 0 ) {
                          perror ( " sendto error " );
                          thread_death();
                          close(sock_data);
                          pthread_exit(NULL);
                        }*/
                        printf("Start sender\n");
                        start_sender(&datagram, sock_data, &clientaddr);







              
            }else if( stringCmpi(datagram.command, "list") == 0 ){

                        

                        //------------------- list body --------------------------


                        printf("list\n" );


                        /*
                         *  eseguo uno shell script che costruisce un file
                         *  contenente l'albero delle directories e dei files
                         *  figli della directory root presente nel ./
                         */
                        printf("Lauch shell script\n");
                        system("./build_tree.sh"); // eseguo lo script

                        datagram_setup_list( &datagram );

                        //try to send the datagram to the client
                        // n = sendto( sock_data ,  &datagram,  sizeof(datagram) , 0 ,
                        //             ( struct sockaddr *)&clientaddr , sizeof( clientaddr ));
                        // if ( n < 0 ) {
                        //   perror ( " sendto error " );
                        //   thread_death();
                        //   close(sock_data);
                        //   pthread_exit(NULL);
                        // }
                        printf("Start sender\n");
                        start_sender(&datagram, sock_data, &clientaddr);

                        free(&datagram);
              
            }else{

              perror("error with the arguments passed\n");
              thread_death();
              close(sock_data);
              pthread_exit(NULL);
            }
        }      

        printf("Exiting from thread child...\n");
        thread_death();
        pthread_exit(NULL);


}



int main(int argc, char *argv[]) {
  
  int	                  sockfd; 						//file descriptor
  fd_set                sockets;
  struct sockaddr_in	  servaddr;
  pthread_t             threads[MAX_THREADS];
  



  /* setting the address */
  memset((void *)&servaddr, 0, sizeof(servaddr));
  // create the udp socket 
  sockfd = udp_socket_init_server( &servaddr, NULL, SERV_PORT );
  if( sockfd == -1 ){
    perror("socket server init error");
    exit(-1);
  }

 
  /* 
   *  finished bind between socket and address
   *  Server can read on this socket
   */


  FD_ZERO(&sockets);         /* Inizializza l’insieme di descrittori dei sockets con l’insieme vuoto */
  FD_SET(sockfd,&sockets);   /* Aggiunge socket all'insieme, mettendo ad 1 il bit */
  fcntl(sockfd,F_SETFL,O_NONBLOCK);    /* rendo il recv non bloccante */
  

  while (1) {
          //    -------------------    Server body
          
          /* 
           * Controllo in lettura lo stato dei socket 
           * Multiplexing dell' I/O
           * il server può gestire tutte le richieste tramite un singolo processo (main)
           * pero attenzione ---> gestire piu client
           * 
          */


          if ( select(sockfd + 1,&sockets,NULL,NULL,NULL) < 0) {
              perror("select()");
              return  -1;
          }


          /*
           *  al ritorno della select controllo 
           *  servo il descrittore che ha causato 
           *  l'uscita creando un thread handler
          */
                           
          if (numthreads < MAX_THREADS) {
              pthread_create(&threads[numthreads],NULL, client_request ,(void *)&sockfd);
          }
          printf("Currently handling client. %d threads in use.\n",numthreads);   
  }
  
  //normal exit
  exit(0);
}
