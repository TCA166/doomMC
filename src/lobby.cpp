#include "lobby.hpp"
#include "player.hpp"

extern "C"{
    #include <stdlib.h>
    #include <pthread.h>
    #include <errno.h>
    #include <string.h>
}

typedef void*(*thread)(void*);

#define timeout {60, 0}

//TODO handle 1.20 change to NBT (root tag no name)
byteArray lobby::createRegistryCodec(){
    size_t size = 85;
    byte* codec = new byte[size];
    codec[0] = TAG_COMPOUND;
    writeBigEndianShort(codec + 1, 0); //no name for root tag
    {//init world world tag
        codec[3] = TAG_COMPOUND;
        writeBigEndianShort(codec + 4, 24); //no name for root tag
        memcpy(codec + 7, "minecraft:dimension_type", 24);
        codec[31] = TAG_INVALID;
    }
    {//biome tag
        codec[32] = TAG_COMPOUND;
        writeBigEndianShort(codec + 33, 24); //no name for root tag
        memcpy(codec + 35, "minecraft:worldgen/biome", 24);
        codec[59] = TAG_INVALID;
    }
    {//chat tag
        codec[60] = TAG_COMPOUND;
        writeBigEndianShort(codec + 61, 20); //no name for root tag
        memcpy(codec + 63, "minecraft:chat_type ", 20);
        codec[83] = TAG_INVALID;
    }
    codec[84] = TAG_INVALID;
    return {codec, size};
}

byteArray lobby::getRegistryCodec(){
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
    this->registryCodec = this->createRegistryCodec();
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
