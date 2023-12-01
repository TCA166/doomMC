#include "lobby.hpp"
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "player.hpp"
#include <string.h>

typedef void*(*thread)(void*);

#define timeout {60, 0}

void* lobby::monitorPlayers(lobby* thisLobby){
    fd_set readfds;
    int maxSd = 0;
    while(true){
        FD_ZERO(&readfds);
        for(int i = 0; i < thisLobby->playerCount; i++){
            if (thisLobby->players[i] == NULL){
                continue;
            }
            int fd = thisLobby->players[i]->getFd();
            FD_SET(fd, &readfds);
            if(fd > maxSd){
                maxSd = fd;
            }
        }
        int activity = select(maxSd + 1, &readfds, NULL, NULL, &thisLobby->monitorTimeout);
        if(activity > 0){
            for(int i = 0; i < thisLobby->playerCount; i++){
                if (thisLobby->players[i] == NULL){
                    continue;
                }
                int fd = thisLobby->players[i]->getFd();
                if(FD_ISSET(fd, &readfds)){
                    packet p = thisLobby->players[i]->getPacket();
                    while(!packetNull(p)){
                        int res = thisLobby->players[i]->handlePacket(&p);
                        printf("res: %d\n", res);
                        if(res < 1){
                            break;
                        }
                        free(p.data);
                        p = thisLobby->players[i]->getPacket();
                    }
                    if(errno != EAGAIN && errno == EWOULDBLOCK){
                        delete thisLobby->players[i];
                        thisLobby->players[i] = NULL;
                    }
                }
            }
        }
        else if((activity < 0) && (errno != EINTR)){
            perror("monitor select");
            return NULL;
        }
        thisLobby->monitorTimeout = timeout;
    }
}

lobby::lobby(unsigned int maxPlayers) : maxPlayers(maxPlayers){
    this->playerCount = 0;
    this->monitorTimeout = timeout; //60 seconds
    this->players = new player*[maxPlayers];
    memset(this->players, 0, sizeof(player*) * maxPlayers);
    if(pthread_create(&this->monitor, NULL, (thread)this->monitorPlayers, this) < 0){
        throw "Failed to create monitor thread";
    }
}

unsigned int lobby::getPlayerCount(){
    return this->playerCount;
}

void lobby::addPlayer(player* p){
    if(this->playerCount >= this->maxPlayers){
        return;
    }
    for(int i = 0; i < this->maxPlayers; i++){
        if(this->players[i] == NULL){
            p->startPlay(i, this);
            this->players[i] = p;
            this->monitorTimeout = {0, 0};
        }
    }
    this->playerCount++;
}

unsigned int lobby::getMaxPlayers(){
    return this->maxPlayers;
}

void lobby::sendMessage(char* message){
    for(int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL){
            continue;
        }
        this->players[i]->sendMessage(message);
    }
}

void lobby::removePlayer(player* p){
    for(int i = 0; i < this->playerCount; i++){
        if(this->players[i] == p){
            this->players[i] = NULL;
            this->playerCount--;
            break;
        }
    }
}
