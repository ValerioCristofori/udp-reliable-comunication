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


int datagram_setup_put( Datagram* datagram_ptr, char** arguments,  FILE *fp ){
        
       /*
		* Return -1 if file is not supported 
		* its length is greater than MAXFILE
		* Else return the lenght of the datagram to send  
		*/
        
        int length, i;
        char ch, ch2;


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
        printf("command length: %ld\n", sizeof(datagram_ptr->command) );

        strcpy( datagram_ptr->filename, arguments[1] );
        datagram_ptr->err = 0;

        // build file of the datagram with file stream
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);  //count the file length
        fseek(fp, 0, SEEK_SET);
	
      	if( length >= MAXFILE ){
            printf("Length of file %d\n", length );
            return -1;
      	}	
        
        datagram_ptr->length_file = length;

        rewind(fp);  //rewind the file pointer to the start position

		//encrypt all char one by one and set in the datagram->file
        for ( i = 0; i < length; i++) {  

            ch = fgetc(fp); 
            ch2 = Cipher(ch); 
            datagram_ptr->file[i] = ch2; 
        }

        datagram_ptr->file[i] = EOF;  //add the end of file byte
        printf("File dimension %d\n", datagram_ptr->length_file);
        

        print_datagram(datagram_ptr);

        return LENGTH_HEADER + datagram_ptr->length_file;

}


int datagram_setup_get( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );  //set command string
        strcpy( datagram_ptr->filename, arguments[1] ); //set filename to get
        datagram_ptr->err = 0;		//no error
        datagram_ptr->length_file = 0;

        

        print_datagram(datagram_ptr);
        
        return LENGTH_HEADER + datagram_ptr->length_file;
        

} 

int datagram_setup_list( Datagram* datagram_ptr, char** arguments ){ 

        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );  //set command string
        datagram_ptr->err = 0;

        datagram_ptr->length_file = 0;

        print_datagram(datagram_ptr);


        return LENGTH_HEADER + datagram_ptr->length_file;
 
        

}

int datagram_setup_exit( Datagram* datagram_ptr, char** arguments ){


        // setupping the struct 
        strcpy( datagram_ptr->command, arguments[0] );
        datagram_ptr->err = 0;

        datagram_ptr->length_file = 0;

        print_datagram(datagram_ptr);


        return LENGTH_HEADER + datagram_ptr->length_file;
 
        

}

int datagram_setup_exit_signal( Datagram* datagram_ptr){


        // setupping the struct 
        datagram_ptr->length_file = 0;
        datagram_ptr->err = 1;  //set error flag to 1 
								//exiting by signal
        

        print_datagram(datagram_ptr);


        return LENGTH_HEADER + datagram_ptr->length_file;
 
        

}