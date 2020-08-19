#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "defines.h"




char Cipher(char ch) 
{ 
	/*
	 * Return the encrypt or decrypt char matches with 'ch'
	 */
    return ch ^ KEY;  // the operator '^' make the XOR betw ch and KEY
					  // KEY must be a char 
}


void decrypt_content(FILE *fp, char* str, int length ){

      int     i;
      char    ch;

      
      for (i = 0; i<length; i++){ 
    
          // Input string into the file 
          // single character at a time 
          ch = Cipher(str[i]);
          fputc(ch, fp); 
     }
}


int datagram_setup_get( Datagram* datagram_ptr, char* filename ){
   
   /*
    * Return -1 if cant open the file
    * Else return the lenght of the datagram to send  
    */
   
   
   FILE*      fp;
   int        length, i;
   char       ch, ch2;

			

          fp = fopen( filename, "r");   //try to open the file
          printf("\nFile Name Received: %s\n", filename); 
          if (fp == NULL){ 
              printf("\nFile open failed!\n"); 
              return -1;
          }
          else
              printf("\nFile Successfully opened!\n");

          // build file of the datagram with file stream
          fseek(fp, 0, SEEK_END);
          length = ftell(fp);  //count the file length
          fseek(fp, 0, SEEK_SET);
          datagram_ptr->length_file = length;

          rewind(fp); //rewind the file pointer to the start position

		  //encrypt all char one by one and set in the datagram->file
          for ( i = 0; i < length; i++) {  
			
              ch = fgetc(fp);  
              ch2 = Cipher(ch); 
              datagram_ptr->file[i] = ch2; 
          }
 
          datagram_ptr->file[i] = EOF;  //add the end of file byte
          printf("File dimension %d\n", datagram_ptr->length_file);

          return LENGTH_HEADER + datagram_ptr->length_file;
}


int datagram_setup_list( Datagram* datagram_ptr){

	/*
	 * Very similar to the get setup case
	 * 
     * Return -1 if cant open the file
     * Else return the lenght of the datagram to send  
     */

    FILE* 		fp;
    char 		ch;
    int 		length;
    int 		i;



          printf("Open file that contains tree\n");
          fp = fopen( "tree", "r");  // try to open the file of the dirs, 'tree'
          if (fp == NULL) {
              printf("\nFile open failed!\n"); 
			  return -1;
          }else
              printf("\nFile Successfully opened!\n");

          fseek(fp, 0, SEEK_END);
          length = ftell(fp);
          fseek(fp, 0, SEEK_SET);

          datagram_ptr->length_file = length;

          rewind(fp);
			
		  
          for ( i = 0; i < length; i++) { 
              ch = fgetc(fp);  
              datagram_ptr->file[i] = ch; 
          }
          printf("File dimension %d\n", datagram_ptr->length_file);
          
          return LENGTH_HEADER + datagram_ptr->length_file;

    

}

int datagram_setup_error( Datagram *datagram_ptr, int error ){

	  /*
	   * Used to say at the client the error state of his call
	   * Return the length of the datagram for the sending
	   */

      // setupping the struct 
      datagram_ptr->length_file = 0;
      datagram_ptr->err = error;     //init the datagram entry 'err' with the specific error code 
									 //defined in the file defines.h

      print_datagram(datagram_ptr);


      return LENGTH_HEADER + datagram_ptr->length_file;
}