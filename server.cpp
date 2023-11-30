#include <iostream>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/random.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>

#define PORT 8080
#define MAX_LISTEN 5

int main(int argc, char *argv[]){
    //socket that accepts new connections
    int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(masterSocket < 0){
        perror("socket");
        return EXIT_FAILURE;
    }
    int opt = true;
    //set master socket to allow multiple connections
    if(setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 ){
        perror("setsockopt");
        return EXIT_FAILURE;
    }
    //master socket address
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };
    address.sin_addr.s_addr = INADDR_ANY;
    size_t addrlen = sizeof(address);
    //bind the master socket to localhost port 8080
    if(bind(masterSocket, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind");
        return EXIT_FAILURE;
    }
    //try to specify maximum of 5 pending connections for the master socket
    if(listen(masterSocket, MAX_LISTEN) < 0){
        perror("listen");
        return EXIT_FAILURE;
    }
    //set of socket descriptors
    fd_set readfds;
    FD_SET(masterSocket, &readfds);
    while(true){
        //wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        int activity = select(1, &readfds, NULL, NULL, NULL);
    
        if((activity < 0) && (errno!=EINTR)){
            perror("select");
            return EXIT_FAILURE;
        }
        //If something happened on the master socket, then its an incoming connection
        if(FD_ISSET(masterSocket, &readfds)){
            int newSocket;
            if ((newSocket = accept(masterSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
                perror("accept");
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}
