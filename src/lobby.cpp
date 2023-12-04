#include "lobby.hpp"
#include "player.hpp"
#include "doom.hpp"

extern "C"{
    #include <stdlib.h>
    #include <stdio.h>
    #include <pthread.h>
    #include <errno.h>
    #include <string.h>
    #include "../cNBT/nbt.h"
}

typedef void*(*thread)(void*);

#define timeout {60, 0}

const byteArray* lobby::getRegistryCodec(){
    return this->registryCodec;
}

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
        else if(activity < 0 && errno != EINTR && errno != EBADF){
            perror("monitor select");
            return NULL;
        }
        thisLobby->monitorTimeout = timeout;
    }
}

lobby::lobby(unsigned int maxPlayers, const byteArray* registryCodec, const struct weapon* weapons, const struct ammo* ammo) : maxPlayers(maxPlayers), weapons(weapons), ammo(ammo), registryCodec(registryCodec){
    this->playerCount = 0;
    this->monitorTimeout = timeout; //60 seconds
    this->players = new player*[maxPlayers];
    memset(this->players, 0, sizeof(player*) * maxPlayers);
    if(pthread_create(&this->monitor, NULL, (thread)this->monitorPlayers, this) < 0){
        throw "Failed to create monitor thread";
    }
}

lobby::lobby(unsigned int maxPlayers, const byteArray* registryCodec) : lobby(maxPlayers, registryCodec, doomWeapons, doomAmmunition){

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
            this->players[i] = p;
            p->startPlay(i, this);
            this->monitorTimeout = {0, 0};
            break;
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

const struct weapon* lobby::getWeapons(){
    return this->weapons;
}

const struct ammo* lobby::getAmmo(){
    return this->ammo;
}
