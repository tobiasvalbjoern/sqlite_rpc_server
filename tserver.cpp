/*
 ============================================================================
 Name        : tserver.c
 Author      : BEEJ's guide to network programming, Tobias, Jens
 Version     : 2.0
 Copyright   : None
 Date 		 : 29/5-2017
 Modified    : 30/1-2018
 Description : This tserver program will accept multiple incoming connections
 and they will echo back the string send from the client.
 ============================================================================
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <pthread.h>
#include <syslog.h>

#include <iostream>

#include "stringtrim.h"
// how many pending connections queue will hold
#define BACKLOG 10
#define BUFSIZE 1024
#define EOT 0x04

void * connection_handling(void *);
void * listen_thread(void *);

enum state {
    notSet, Set
};

//Stores the filedescriptors. listen on sock_fd
int sockfd;
std::string(*handle_callback)(std::string) ;

void tserver_init(const char * interface, const char *port, std::string(*handler)(std::string)) {

    handle_callback = handler;
    //Linked lists. Hints is to store our settings. servinfo is to collect
    //information about a particular host name.
    //p i used to scroll through servinfo.
    struct addrinfo hints, *servinfo, *p;

    //Initialize hints.
    //Some fields we need to set.
    //All the other fields in the structure pointed to by hints must
    //contain either 0 or a NULL pointer,  as  appropriate.
    //there needs to be zero's for the "getaddrinfo" function.
    memset(&hints, 0x00, sizeof (hints));

    //The  hints  argument  points to an addrinfo structure that specifies
    //criteria for selecting the socket address
    //structures returned in the list pointed to by res

    //It can use both IPv4 or IPv6
    hints.ai_family = AF_UNSPEC;
    //For streaming socket. Write SOCK_DGRAM for datagram.
    hints.ai_socktype = SOCK_STREAM;
    //Use my IP
    hints.ai_flags = AI_PASSIVE;

    //Allocates and initialize a linked list (servinfo)
    //of addrinfo structures. One for each network address that matches
    //node and service and returns a pointer to
    //the start of that list to servinfo.
    //Each contains Internet address that can be specified in a
    //call to bind or connect
    //DNS and service name lookup , fills the structs we need.
    //Returns zero on success.
    int rv;
    if ((rv = getaddrinfo(interface, port, &hints, &servinfo)) != 0) {
        //gai_strerror returns error code from getaddrinfo.
        syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    //Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        //socket creates an endpoint for communication. Returns descriptor.
        //-1 on error
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
                == -1) {
            syslog(LOG_ERR, "Cannot create socket");
            continue;
        }
        //every packet with destination p->ai_addr should be forwarded to
        //sockfd.socket needs to be associated with a port on local machine.
        //bind sets errno to the error if it fails.
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            syslog(LOG_ERR, "Cannot bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    //If the linked list has reached the end without binding.
    if (p == NULL) {
        syslog(LOG_ERR, "Sockfd Could not associate with port. Closing down");
        exit(1);
    }

    //Listen for connections on a socket.
    //sockfd is marked as passive, one used to accept incoming connection
    //requests using accept.
    //listen also sets errno on error.
    if (listen(sockfd, BACKLOG) == -1) {
        syslog(LOG_ERR, "Error on listen");
        exit(1);
    }

    syslog(LOG_INFO, "server: waiting for connections...");

    pthread_attr_t attr;

    pthread_attr_init(&attr);

    // Collect tid's here
    pthread_t threads;

    //&threads = unique identifier for created thread.
    //connection_handling = start routine
    // (void*) argument for our start routine, you can send one as a
    //void pointer.
    int rc = pthread_create(&threads, &attr, listen_thread, NULL);
    if (rc) {
        syslog(LOG_ERR, "Couldn't create listen thread");
        exit(-1);
    }

    pthread_attr_destroy(&attr);
}

void * listen_thread(void * p) {
    //Endless loop that awaits connections
    while (1) {
        //storage for the size of the connected address.
        // connector's address information
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof (their_addr);

        //intptr_t is an integer with the same size as a pointer on the system
        intptr_t new_fd = accept(sockfd, (struct sockaddr *) &their_addr,
                &sin_size);
        if (new_fd == -1) {
            syslog(LOG_ERR, "Could not accept");
            continue;
        }

        syslog(LOG_INFO, "Accepted incomming connection");

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        // Collect tid's here
        pthread_t threads;

        //&threads = unique identifier for created thread.
        //connection_handling = start routine
        // (void*) argument for our start routine, you can send one as a
        //void pointer. We send the file descriptor for the new connection.
        int rc = pthread_create(&threads, &attr, connection_handling,
                (void*) new_fd);
        if (rc) {
            syslog(LOG_ERR, "Couldn't create thread: %d", rc);
            exit(1);
        }

        pthread_attr_destroy(&attr);
    }

}

void * connection_handling(void * new_fd) {
    enum state done = notSet;

    //Cast back fd to an integer.
    intptr_t fd = (intptr_t) new_fd;

    char buf_out[BUFSIZE];
    //No worries about strcpy, there are plenty of space
    //in the input buffer.
    sprintf(buf_out, "CONNECTED with ID: %d", new_fd);
    send(fd, buf_out, strlen(buf_out), 0);

    while (!done) {
        char buf_in[BUFSIZE];

        memset(buf_in, 0x00, strlen(buf_in));

        int n = recv(fd, buf_in, sizeof (buf_in), 0);


        if (n < 0) {
            syslog(LOG_ERR, "Could not read from socket");
            done = Set;
            break;
        }

        if (n == 0) {
            syslog(LOG_INFO, "Peer has performed orderly shutdown");
            done = Set;
            break;
        }

        syslog(LOG_INFO, "Received %d bytes: %s", n, buf_in);

        //End of file
        if (buf_in[0] == EOT) {
            done = Set;
            syslog(LOG_INFO, "Peer has performed orderly shutdown");
            //break out of the loop and test on the main while loop
            break;
        }
     
        std::string input(buf_in);
        trim(input);
        std::string output = handle_callback(input);
        if(output.length() > 1)
        {
            n = send(fd, output.c_str(), output.length(), 0);
            if (n < 0) {
                syslog(LOG_ERR, "Could not write to socket");

                done = Set;
                //break out of the loop and test on the main while loop
                break;
            }
             syslog(LOG_INFO, "Sent %d bytes: %s", n, output.c_str());
        }
    }
    close(fd);
    return NULL;
}
