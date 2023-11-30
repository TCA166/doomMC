#include <iostream>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/random.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>

#include "player.hpp"
#include "lobby.hpp"

extern "C" {
    #include "networkingMc.h"
    #include "../cJSON/cJSON.h"
}

#define PORT 8080
#define MAX_LISTEN 5

#define MAX_LOBBIES 1

#define MAX_CLIENTS 100

//Special struct that holds the socket and the state of the client
struct client{
    int fd;
    byte state;
};

/*!
 @brief Disconnects a client
 @param c the client to disconnect
*/
static inline void disconnectClient(struct client* c){
    close(c->fd);
    c->fd = 0;
}

/*!
 @brief Handles all the login related packets from a client
 @param p the packet to handle
 @param c the client that sent the packet
 @return -1 on error, 0 on success
*/
static inline int handlePacket(packet* p, struct client* c){
    int offset = 0;
    switch(c->state){
        case NONE_STATE:{
            if(p->packetId != HANDSHAKE){
                close(c->fd);
                c->fd = 0;
                break;
            }
            int32_t protocolVersion = readVarInt(p->data, &offset);
            int32_t serverAddressLength = readVarInt(p->data, &offset);
            offset += serverAddressLength + sizeof(unsigned short);
            int32_t nextState = readVarInt(p->data, &offset);
            c->state = nextState;
            break;
        }
        case STATUS_STATE:{
            if(p->packetId == STATUS_REQUEST){
                //TODO finish the status request
                byte data[256];
                char json[] = "{\"version\":{\"name\":\"N/A\",\"protocol\":754},\"players\":{\"max\":100,\"online\":0,\"sample\":[]},\"description\":{\"text\":\"Hello world\"}}";
                if(sendPacket(c->fd, writeString(data, json, sizeof(json)), STATUS_RESPONSE, (const byte*)data, NO_COMPRESSION) < 0){
                    return -1;
                }
            }
            else if(p->packetId == PING_REQUEST){
                int64_t val = readLong(p->data, &offset);
                if(sendPacket(c->fd, sizeof(val), PING_RESPONSE, (byte*)&val, NO_COMPRESSION) < 0){
                    return -1;
                }
            }
            else{
                disconnectClient(c);
            }
            break;   
        }
        case LOGIN_STATE:{
            break;
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    //socket that accepts new connections
    int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(masterSocket < 0){
        perror("socket");
        return EXIT_FAILURE;
    }
    //Array of currently processed clients
    struct client clients[MAX_CLIENTS] = {};
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
            int sd = clients[i].fd;
            //if valid socket descriptor then add to read list
            if(sd > 0){
                FD_SET(sd, &readfds);
            }
            //highest file descriptor number, need it for the select function
            if(sd > maxSd){
                maxSd = sd;
            }
        }
        //wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        int activity = select(maxSd + 1, &readfds, NULL, NULL, NULL);
        if((activity < 0) && (errno!=EINTR)){
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
                if(clients[i].fd <= 0){
                    clients[i].fd = newSocket;
                    clients[i].state = 0;
                    break;
                }
            }
            if(i == MAX_CLIENTS){
                close(newSocket);
            }
        }
        //do IO on a different socket
        for(int i = 0; i < MAX_CLIENTS; i++){
            byte b;
            if(FD_ISSET(clients[i].fd, &readfds)){
                packet p = readPacket(clients[i].fd, NO_COMPRESSION);
                while(!packetNull(p)){
                    printf("packetId:%d\n", p.packetId);
                    if(handlePacket(&p, clients + i) < 0){
                        perror("handlePacket");
                        return EXIT_FAILURE;
                    }
                    free(p.data);
                    p = readPacket(clients[i].fd, NO_COMPRESSION);
                }
                if(errno != EAGAIN && errno != EWOULDBLOCK){
                    disconnectClient(clients + i);
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
