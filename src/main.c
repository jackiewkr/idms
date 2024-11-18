/**
 * advertiser.c
 * ============
 * Program for broadcasting availability to other clients, along with the
 * logging of available clients.
 *
 * By Jacqueline W., 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>

#define ADVERTISING_PORT ( 7448 )
#define FORGET_TIMEOUT   ( 5 * 60 )
#define MAX_HOSTNAME_LEN ( 32 )

/* Flags */
static volatile int should_quit = 0;

/* Handle SIGINTs to quit cleanly */
void sigint_handler( int sig )
{
        should_quit = 1;
}

/* Thread for listening to socket for any new messages */
void* listen_loop( void* param )
{
        int sock = *(int*)param;
        while ( !should_quit )
        {
                char* buf = malloc( MAX_HOSTNAME_LEN + 1 );
                recvfrom( sock, buf, MAX_HOSTNAME_LEN + 1, 0, NULL, NULL );
                printf( "Received: %s\n", buf );
        }
        printf("Listen Thr finished...\n");
        return NULL;
}

int main( void )
{
        /* Catch signals */
        signal( SIGINT, sigint_handler );

        /* Set up UDP socket for reads/writes */
        int sock = socket( AF_INET, SOCK_DGRAM, 0 );

        /* Set up broadcast address */
        struct sockaddr_in bc_addr;
        bc_addr.sin_family = AF_INET;
        bc_addr.sin_port = ADVERTISING_PORT;
        bc_addr.sin_addr.s_addr = INADDR_BROADCAST;

        /* Set up listen address & binding */
        struct sockaddr_in ls_addr;
        ls_addr.sin_family = AF_INET;
        ls_addr.sin_port = ADVERTISING_PORT;
        ls_addr.sin_addr.s_addr = INADDR_ANY;
        int retval = bind( sock, &ls_addr, INET_ADDRSTRLEN );

        if ( retval == -1 ) {
                should_quit = 1;
                printf( "ERROR: Failed to bind to port!\n" );
        }

        /* Allow broadcasting */
        int active = 1;
        setsockopt( sock, SOL_SOCKET, SO_BROADCAST, &active, sizeof(active) );

        /* Create pthread for listening */
        pthread_t listen_id;
        pthread_create( &listen_id, NULL, &listen_loop, (void*)&sock );

        /* Broadcast message every FORGET_TIMEOUT */
        char* hostname = malloc( MAX_HOSTNAME_LEN + 1 );  //+1 for null term
        int hostname_sz = gethostname( hostname, MAX_HOSTNAME_LEN + 1 );
        while ( !should_quit ) {
                sendto( sock, hostname, hostname_sz, 0, &bc_addr,
                        INET_ADDRSTRLEN );
                printf( "Sent hostname, now sleeping...\n" );
                
                time_t end_sleep_time = time(NULL) + FORGET_TIMEOUT - 1;
                while ( !should_quit && ( time(NULL) < end_sleep_time ) ) {
                }
        }

        /* Wait for listen thread to finish too */
        pthread_join( listen_id, NULL );

        /* Clean up nicely */
        close( sock );

        printf( "Finished..\n" );

        return 0;
}
