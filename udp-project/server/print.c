#include <stdio.h>
#include <stdlib.h>

#include "defines.h"



void print_datagram( Datagram *datagram_ptr){

      printf("\nCommand: %s, sizeof(): %ld\n", datagram_ptr->command, sizeof(datagram_ptr->command) );
      printf("Filename: %s, sizeof(): %ld\n", datagram_ptr->filename, sizeof(datagram_ptr->filename) );
      printf("File size: %d, sizeof(): %ld\n", datagram_ptr->length_file, sizeof(datagram_ptr->length_file) );
      printf("File content: %s, sizeof(): %ld\n", datagram_ptr->file, sizeof(datagram_ptr->file) );
      printf("Error message: %s, sizeof(): %ld\n", datagram_ptr->error_message, sizeof(datagram_ptr->error_message) );
      printf("Code error: %d, sizeof(): %ld\n", datagram_ptr->err, sizeof(datagram_ptr->err) );
      
}

void print_state_sender(State *s){
	printf("----STATE SENDER----\n");
	printf("Window size: %d\n", window);
	printf("Tries number: %d\n", s->tries);
	printf("Send base: %d\n", s->send_base);
	printf("Next sequence number: %d\n", s->next_seq_no);
	printf("Bytes reads: %d\n", s->byte_reads );
	printf("Expected sequence number: %d\n", s->expected_seq_no);
}