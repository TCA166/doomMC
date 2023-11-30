#include <iostream>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/random.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>

#include <string.h>

#include "player.hpp"
#include "lobby.hpp"
#include "client.hpp"

#define PORT 8080
#define MAX_LISTEN 5

#define MAX_LOBBIES 1

#define MAX_CLIENTS 100

int main(int argc, char *argv[]){
    //socket that accepts new connections
    int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(masterSocket < 0){
        perror("socket");
        return EXIT_FAILURE;
    }
    //Array of currently processed clients
    client* clients[MAX_CLIENTS];
    memset(clients, 0, MAX_CLIENTS * sizeof(client*));
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
    lobby lobbies[MAX_LOBBIES] = {lobby(10)};
    int maxSd = masterSocket;
    //set of socket descriptors
    fd_set readfds;
    while(true){
        //clear the socket set in case we disconnected with somebody along the line
        FD_ZERO(&readfds);
        //add master socket to set
        FD_SET(masterSocket, &readfds);
        //readd all the remaining clients to the set
        for(int i = 0; i < MAX_CLIENTS; i++){
            //socket descriptor
            if(clients[i] != NULL){
                int sd = clients[i]->getFd();
                //if valid socket descriptor then add to read list
                if(sd > 0){
                    FD_SET(sd, &readfds);
                }
                //highest file descriptor number, need it for the select function
                if(sd > maxSd){
                    maxSd = sd;
                }
            }
        }
        //wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        int activity = select(maxSd + 1, &readfds, NULL, NULL, NULL);
        if((activity < 0) && (errno != EINTR)){
            perror("select");
            return EXIT_FAILURE;
        }
        //If something happened on the master socket, then its an incoming connection
        if(FD_ISSET(masterSocket, &readfds)){
            int newSocket;
            if ((newSocket = accept(masterSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0){
                perror("accept");
                return EXIT_FAILURE;
            }
            //set newSocket to non blocking
            if(fcntl(newSocket, F_SETFL, fcntl(newSocket, F_GETFL, 0) | O_NONBLOCK) < 0){
                perror("fcntl");
                return EXIT_FAILURE;
            }
            //add new socket to list
            int i;
            for(i = 0; i < MAX_CLIENTS; i++){
                if(clients[i] == NULL){
                    clients[i] = new client(newSocket);
                    break;
                }
            }
            if(i == MAX_CLIENTS){
                close(newSocket);
            }
        }
        //do IO on a different socket
        for(int i = 0; i < MAX_CLIENTS; i++){
            if(clients[i] == NULL){
                continue;
            }
            byte b;
            int thisFd = clients[i]->getFd();
            if(FD_ISSET(thisFd, &readfds)){
                packet p = clients[i]->getPacket();
                int res = 1;
                while(!packetNull(p)){
                    res = clients[i]->handlePacket(&p);
                    if(res < 1){
                        break;
                    }
                    free(p.data);
                    p = clients[i]->getPacket();
                }
                if(errno != EAGAIN && errno != EWOULDBLOCK){
                    delete clients[i];
                    clients[i] = NULL;
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
