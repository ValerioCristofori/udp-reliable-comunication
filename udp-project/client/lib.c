#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>

int numthreads = 0;
pthread_mutex_t threadwatch = PTHREAD_MUTEX_INITIALIZER;
char *OOSMSG = "Maximum connections exceeded.\n";

#define noblock(sck) (fcntl(sck,F_SETFL,O_NONBLOCK)) 
#define DSZ 256
#define ACREDLEN 20
#define MAXTHREADS 20


typedef struct _assoc_data_ {
int msgnum;
short data_type_id;
char reserved;
char assoc_auth_creds[ACREDLEN];
char data[DSZ];
} APKT; 

void decrement_threadcount(void) {
                   pthread_mutex_lock(&threadwatch);
                   numthreads--;
                   pthread_mutex_unlock(&threadwatch);
}

void increment_threadcount(void) {
                pthread_mutex_lock(&threadwatch);
                numthreads++;
                pthread_mutex_unlock(&threadwatch);
}

void print_apkt(APKT dta) {

               printf("Msgnum = %d, data_type_id = %d,reserved = %c\n \
                      assoc_auth_creds = %s, data = %s\n",dta.msgnum,dta.data_type_id,dta.reserved,dta.assoc_auth_creds,dta.data);
}
                       
void *handleclient(void *sck) {
int privsock, tsz, sock;
short syn_ack = 0x11;
APKT rdata;
struct sockaddr_in relation, peer;

                pthread_detach(pthread_self());
                memcpy(&sock,sck,sizeof(sock));
                bzero(&rdata,sizeof(rdata));
                tsz = sizeof(relation);

                
                increment_threadcount();
                /*reading data like the below is broken across various machine specific boundaries, byte ordering, word size, etc..buyer beware*/
                if (recvfrom(sock,&rdata,sizeof(rdata),0,(struct sockaddr *)&relation,&tsz) <= 0) {
                   perror("recvfrom()");
                   decrement_threadcount();
                   pthread_exit(NULL);
                }

                print_apkt(rdata);
                if ( (privsock = retudpsrv("0.0.0.0",0)) < 0) {decrement_threadcount(); pthread_exit(NULL);}
                /*for udp concurrency one should bind an ephemeral port and re-negotiate with the client for communication on the new port->address. One should also find the interface 
                 that the original packet arrived on and only bind this address. */
                noblock(privsock);
                sendto(privsock,&syn_ack,sizeof(syn_ack),0,(struct sockaddr *)&relation,tsz); /*acknowledge data receipt*/
                /*process data in rdata, verify peer credentials and transaction signature/viability and finally send back result to client here*/
                close(privsock);
                decrement_threadcount();
                pthread_exit(NULL);
}
                      

int retudpsrv(char *addr, int pno) {
int sck, option = 1;
struct sockaddr_in srv;

             bzero(&srv,sizeof(srv));
             srv.sin_addr.s_addr = inet_addr(addr);
             srv.sin_port = htons(pno);
             srv.sin_family = AF_INET;
       
             if ( (sck = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
                perror("socket()");
                return -1;
             }

             setsockopt(sck,SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
            /*could and should use IP options for IF and RECVADDR here for recvmsg, but this is more complicated than desirable for an example*/
             noblock(sck);
             
             if (bind(sck,(struct sockaddr *)&srv,sizeof(srv)) < 0) {
                perror("bind()");
                return -1;
             }
             return sck;
}

int main(int argc, char **argv) {
int vopt,sock;
fd_set fds;
pthread_t tids[MAXTHREADS];


           if (argc != 3) {printf("Please specify:\n1) Address to bind\n2)Port number to bind\n"); exit(1);}
           if ( (sock = retudpsrv(argv[1],atoi(argv[2]))) < 0) {return -1;}
           
           FD_ZERO(&fds);
           FD_SET(sock,&fds);
           noblock(sock);

          while (1) {
                    if ( (vopt = select(sock + 1,&fds,NULL,NULL,NULL)) < 0) {
                       perror("select()");
                       return  -1;
                    }
                    /*select may return a number of times before data becomes available to the client handler..this causes multiple thread creation
                     and overhead..with non-blocking i/o and quick exits for bad reads it's fairly painless but this is still bad design in a production
                    server*/ 
                    if (numthreads >= MAXTHREADS) {continue;}
                    pthread_create(&tids[numthreads],NULL,handleclient,(void *)&sock);
                    printf("Currently handling client. %d threads in use.\n",numthreads);
          }
}