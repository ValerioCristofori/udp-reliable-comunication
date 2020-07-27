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
#define ERROR_FILE_DOESNT_EXIST  "Error: File does not exist."
#define ERROR_SHELL_SCRIPT "Error: Server is working bad"
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


int datagram_setup_get( Datagram* datagram_ptr, int last_size, char* filename ){
   
   FILE*      fp;
   int length = 0, i;
   char ch, ch2;



          fp = fopen( filename, "r"); 
          printf("\nFile Name Received: %s\n", filename); 
          if (fp == NULL){ 
              printf("\nFile open failed!\n"); 
              return -1;
          }
          else
              printf("\nFile Successfully opened!\n");

          // costruisco il file del datagram attraverso il file stream
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
          return datagram_ptr->datagram_size;
}


int datagram_setup_list( Datagram* datagram_ptr){

    FILE* fp;
    char ch;
    int length = 0;
    int i;



          printf("Open file that contains tree\n");
          fp = fopen( "tree", "r");  // prova ad aprire il file contenente l'albero
          if (fp == NULL) 
              printf("\nFile open failed!\n"); 
          else
              printf("\nFile Successfully opened!\n");

          while( (ch = fgetc(fp)) != EOF ){
              length++;
          }

          datagram_ptr->length_file = length + 1;

          rewind(fp);

          for ( i = 0; i < length; i++) { 
              ch = fgetc(fp);  
              datagram_ptr->file[i] = ch; 
          }
          datagram_ptr->file[i] = EOF;
          printf("File dimension %d\n", datagram_ptr->length_file);
          datagram_ptr->datagram_size = sizeof(*datagram_ptr);
          return datagram_ptr->datagram_size;

    

}


int filename_to_path( char* filename, char* path ){
    FILE *p;
    char command[FILENAME_LENGTH + 16];
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


void manage_error_signal(Datagram *datagram_ptr, int sockfd ){
      
      switch(datagram_ptr->err){

          case 1:
          default:
                
                printf("The client finished through signal CTRL+C\nExiting from the thread child\n" );
                thread_death();
                close(sockfd);
                pthread_exit(NULL);


      }

}




void setup_error_message( Datagram* datagram_ptr, const char *message ){
    datagram_ptr->err = 1;
    strcpy( datagram_ptr->error_message, message );
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


void print_buff_datagram( Datagram* datagram_ptr ){
      printf("command : %s, filename: %s\n", datagram_ptr->command, datagram_ptr->filename );
}




void *client_request( void *sockfd ){

  struct  sockaddr_in     clientaddr, relation;
  Datagram                *datagram_ptr;
  int                     n, sock, sock_data, fd;
  int                     client_port, tmp, ret, size, size_temp;
  socklen_t               len;
  FILE*                   fp;
  pthread_t               whoami = pthread_self();
  char                    decrypted_string[MAXFILE];
  char                    path_file[40];
  char                    path[MAXLINE];
  char                    *filename;
  short                   syn;

        printf("Created a thread handler\n");  
        /* instead call the pthread_join func */
        pthread_detach(whoami);    /* when a detached thread terminates releases his resources */

        thread_birth();

        memcpy(&sock,sockfd,sizeof(sock));


        len = sizeof(relation);

        n = recvfrom( sock,  &syn,  sizeof(syn),  0, (struct sockaddr *)&relation,  &len );
        if( n <= 0 ){
            perror( "recvfrom error" );
            thread_death();
            pthread_exit(NULL);
        } 
        printf("Syn received: %u\n", syn );

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
            
            //malloc datagram and clear
            datagram_ptr = malloc( sizeof(*datagram_ptr) );
            memset( datagram_ptr, 0, sizeof(*datagram_ptr));


            printf("Start receiver\n");
            size = start_receiver(datagram_ptr, sock_data, &clientaddr, 0.1);
            if( size == -1 ){
                  manage_error_signal(datagram_ptr, sock_data);
            }

            print_datagram(datagram_ptr);


            /*if( datagram_ptr->die_sig == 1 ){
                printf("The client finished through signal CTRL+C\nExiting from the thread child\n" );
                thread_death();
                close(sock_data);
                pthread_exit(NULL);
            }*/

            if( strcmp( datagram_ptr->command, "put") == 0 ){

                        

                        //------------------- put body --------------------------
                        

                        printf("put\n" );

                        decrypt_string( decrypted_string, datagram_ptr->file, datagram_ptr->length_file );
                        printf("%s\n", decrypted_string );

                        filename = malloc((datagram_ptr->length_filename + 1)*sizeof(char));
                        strncpy(filename, datagram_ptr->filename ,datagram_ptr->length_filename);
                        sprintf(path_file, "root/%s", filename);
                        

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

                        printf("Success on download file %s\n", filename );


                        free(datagram_ptr);
                        free(filename);





            }else if( strcmp(datagram_ptr->command, "get") == 0 ){
              
                        

                        //------------------- get body --------------------------


                        printf("get\n");

                        /*  --------------------------------------------------------------------------------
                         *  cambiare il path del file ---------> trovare il file nell'albero e aggiungere dir padri
                         */
                        printf("%d\n", datagram_ptr->length_filename );
                        filename = malloc((datagram_ptr->length_filename + 1)*sizeof(char));
                        strncpy(filename, datagram_ptr->filename ,datagram_ptr->length_filename);

                        //controllo che il file sia presente nell'albero delle directories se si faccio il setup del datagram
                        ret = filename_to_path(filename, path);
                        if( ret == 1 ){
                            setup_error_message( datagram_ptr, ERROR_SHELL_SCRIPT );
                            thread_death();
                            close(sock_data);
                            pthread_exit(NULL);
                        }else if( ret == -1 ){
                            printf("send error message to the client\n");
                            setup_error_message( datagram_ptr, ERROR_FILE_DOESNT_EXIST );

                        }else{
                            printf("path file: %s\n", path );
                            if( (size = datagram_setup_get( datagram_ptr, size_temp, path ) ) == -1 ){  /* -1 if file doesn't exist */
                                thread_death();
                                close(sock_data);
                                pthread_exit(NULL);
                            }else{

                                printf("Il contenuto del file '%s': %s\n", path, datagram_ptr->file );

                            }

                        }  
                        
                        printf("Start sender\n");
                        start_sender(datagram_ptr, size, sock_data, &clientaddr);


                        free(datagram_ptr);
                        free(filename);




              
            }else if( strcmp(datagram_ptr->command, "list") == 0 ){

                        

                        //------------------- list body --------------------------


                        printf("list\n" );


                        /*
                         *  eseguo uno shell script che costruisce un file
                         *  contenente l'albero delle directories e dei files
                         *  figli della directory root presente nel ./
                         */
                        printf("Lauch shell script\n");
                        system("./build_tree.sh"); // eseguo lo script

                        size = datagram_setup_list( datagram_ptr );


                        printf("Start sender\n");
                        start_sender(datagram_ptr, datagram_ptr->datagram_size, sock_data, &clientaddr);

                        free(datagram_ptr);

              
            }else if( strcmp(datagram_ptr->command, "exit") == 0 ){

                        //------------------- exit body --------------------------


                        printf("exit\n");


                        printf("Exiting from thread child...\n");
                        thread_death();
                        close(sock_data);
                        pthread_exit(NULL);

            }
            else{

              perror("error with the arguments passed\n");
              thread_death();
              close(sock_data);
              pthread_exit(NULL);
            }
        }      

        printf("Exiting from thread child...\n");
        thread_death();
        close(sock_data);
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
