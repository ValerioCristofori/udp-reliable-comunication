#pragma once


/* utils functions */
extern ssize_t FullWrite ( int fd , const void * buf , size_t count );

extern int udp_socket_init_server( struct sockaddr_in*   addr,  char*   address, int   num_port );

extern int udp_socket_init_client( struct sockaddr_in*   addr,  char*   address, int   num_port );

extern char** str_split(char* a_str, const char a_delim);

extern int split( const char *str, char c, char ***arr );