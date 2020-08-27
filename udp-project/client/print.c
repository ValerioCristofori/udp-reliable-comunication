#include "defines.h"


void print_datagram( Datagram *datagram_ptr){
        yellow();
        printf("\n------------- DATAGRAM --------------\n");
        blue();
      	printf("\nCommand:       %s, sizeof(): %ld\n", datagram_ptr->command, sizeof(datagram_ptr->command) );
      	printf("Filename:      %s, sizeof(): %ld\n", datagram_ptr->filename, sizeof(datagram_ptr->filename) );
      	printf("File size:     %d, sizeof(): %ld\n", datagram_ptr->length_file, sizeof(datagram_ptr->length_file) );
      	//printf("File content:  %s, sizeof(): %ld\n", datagram_ptr->file, sizeof(datagram_ptr->file) );
      	printf("Error message: %s, sizeof(): %ld\n", datagram_ptr->error_message, sizeof(datagram_ptr->error_message) );
      	printf("Code error:    %d, sizeof(): %ld\n", datagram_ptr->err, sizeof(datagram_ptr->err) );
        reset_color();
}


void print_state_sender(State  *state_send){
    yellow();
		printf("\n-------------STATE SENDER--------------\n");
    blue();
		printf("Window size:                %d\n", window);
		printf("Tries number:               %d\n", state_send->tries);
		printf("Send base:                  %d\n", state_send->send_base);
		printf("Next sequence number:       %d\n", state_send->next_seq_no);
		printf("Expected sequence number:   %d\n", state_send->expected_seq_no);
    reset_color();
}


void print_file( char *file, int length ){
	
  /*
   *  Print the content of the 'file'
   *  Used to print the list of the files
   */
	
  char ch;

  printf("\n\n");

  for (int i = 0; i < length; i++) { 
        ch = file[i];
        printf("%c", ch ); 
        if (ch == EOF){ 
            return;
        } 
      
      }
  printf("\n\n");
}


void print_error_datagram(Datagram *datagram_ptr, int sockfd ){
      
      /*
       *  Handler for the error messages
       *  through error code and the cases
       *  define in the file defines.h
       */
      
      switch(datagram_ptr->err){
          

          case 2:
                red();
                printf("%s\n", ERROR_FILE_DOESNT_EXIST );
                break;

          case 3:
                red();
                printf("%s\n", ERROR_SHELL_SCRIPT );
                break;

          case 4:
                red();
                printf("%s\n", ERROR_SIGINT_SERVER );
                reset_color();
                exit(-1);

          case 5:
                red();
                printf("%s\n", ERROR_TOO_MANY_MATCHES );
				        break;


      }
      reset_color();

}