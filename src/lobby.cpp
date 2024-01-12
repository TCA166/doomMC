#include "lobby.hpp"
#include "player.hpp"
#include "doom.hpp"
#include <spdlog/spdlog.h>

extern "C"{
    #include <stdlib.h>
    #include <stdio.h>
    #include <pthread.h>
    #include <errno.h>
    #include <string.h>
    #include "../cNBT/nbt.h"
    #include <unistd.h>
    #include <sys/epoll.h>
}

const byteArray* lobby::getRegistryCodec() const{
    return this->registryCodec;
}

//this is static so 'this' doesn't work
void* lobby::monitorPlayers(lobby* thisLobby){
    epoll_event* events = new epoll_event[thisLobby->maxPlayers];
    while(true){
        int activity = epoll_wait(thisLobby->epollFd, events, thisLobby->maxPlayers, infiniteTime);
        if(activity > 0){
            for(int i = 0; i < activity; i++){
                if(events[i].data.fd == thisLobby->epollPipe[0]){
                    char buf[1];
                    read(thisLobby->epollPipe[0], buf, 1);
                    continue;
                }
                player* p = (player*)events[i].data.ptr;
                if(events[i].events & EPOLLRDHUP){
                    thisLobby->removePlayer(p);
                    delete p;
                    continue;
                }
                packet pack = p->getPacket();
                int res = p->handlePacket(&pack);
                free(pack.data);
            }
        }
        else if(activity < 0 && errno != EINTR){
            perror("epoll_wait");
            return NULL;
        }
    }
    delete[] events;
}

void* lobby::mainLoop(lobby* thisLobby){
    //do stuff 20 times a second
    while(true){
        usleep(50000);
        for(unsigned int i = 0; i < thisLobby->maxPlayers; i++){
            if(thisLobby->players[i] == NULL){
                continue;
            }
            thisLobby->players[i]->keepAlive();
        }
    }
}

lobby::lobby(unsigned int maxPlayers, const byteArray* registryCodec, const struct weapon* weapons, const struct ammo* ammo, const map* lobbyMap) : maxPlayers(maxPlayers), registryCodec(registryCodec), weapons(weapons), ammo(ammo), lobbyMap(lobbyMap){
    this->playerCount = 0;
    this->players = new player*[maxPlayers];
    memset(this->players, 0, sizeof(player*) * maxPlayers);
    this->epollFd = epoll_create1(0);
    if(this->epollFd < 0){
        perror("epoll_create1");
        throw std::error_code(errno, std::generic_category());
    }
    if(pipe(this->epollPipe) < 0){
        perror("pipe");
        throw std::error_code(errno, std::generic_category());
    }
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = this->epollPipe[0];
    if(epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->epollPipe[0], &event) < 0){
        perror("epoll_ctl");
        throw std::error_code(errno, std::generic_category());
    }
    if(pthread_create(&this->monitor, NULL, (thread)this->monitorPlayers, this) < 0){
        throw std::error_code(errno, std::generic_category());
    }
    if(pthread_create(&this->main, NULL, (thread)this->mainLoop, this) < 0){
        throw std::error_code(errno, std::generic_category());
    }
}

lobby::lobby(unsigned int maxPlayers, const byteArray* registryCodec, const map* lobbyMap) : lobby(maxPlayers, registryCodec, doomWeapons, doomAmmunition, lobbyMap){

}

lobby::~lobby(){
    for(unsigned int i = 0; i < this->maxPlayers; i++){
        if(this->players[i] != NULL){
            delete[] this->players[i];
        }
    }
    delete[] this->players;
    close(this->epollFd);
}

unsigned int lobby::getPlayerCount() const{
    return this->playerCount;
}

void lobby::addPlayer(player* p){
    if(this->playerCount >= this->maxPlayers){
        return;
    }
    this->playerCount++;
    for(unsigned int i = 0; i < this->maxPlayers; i++){
        if(this->players[i] == NULL){
            this->players[i] = p;
            p->setIndex(i);
            epoll_event event;
            event.events = EPOLLIN | EPOLLRDHUP;
            event.data.ptr = p;
            if(epoll_ctl(this->epollFd, EPOLL_CTL_ADD, p->getFd(), &event) < 0){
                perror("epoll_ctl");
                throw std::error_code(errno, std::generic_category());
            }
            write(this->epollPipe[1], "\0", 1);
            p->startPlay(i, this);
            break;
        }
    }
}

unsigned int lobby::getMaxPlayers() const{
    return this->maxPlayers;
}

void lobby::sendMessage(char* message){
    for(unsigned int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL){
            continue;
        }
        this->players[i]->sendMessage(message);
    }
}

void lobby::removePlayer(player* p){
    if(epoll_ctl(this->epollFd, EPOLL_CTL_DEL, p->getFd(), NULL) < 0){
        perror("epoll_ctl");
        throw std::error_code(errno, std::generic_category());
    }
    for(unsigned int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL || this->players[i] == p){
            continue;
        }
        this->players[i]->removePlayer(p);
    }
    this->players[p->getIndex()] = NULL;
    this->playerCount--;
    spdlog::debug("Removing player {}({}) from lobby", p->getUUID(), p->getIndex());
}

const struct weapon* lobby::getWeapons() const{
    return this->weapons;
}

const struct ammo* lobby::getAmmo() const{
    return this->ammo;
}

const map* lobby::getMap() const{
    return this->lobbyMap;
}

const player* lobby::getPlayer(int n) const{
    return this->players[n];
}

void lobby::updatePlayerPosition(const player* p, int x, int y, int z){
    for(unsigned int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL || this->players[i] == p){
            continue;
        }
        if(y <= 0){
            y = 1;
        }
        spdlog::debug("Moving player {}({}) position by ({}, {}, {})", p->getUUID(), p->getIndex(), x, y, z);
        this->players[i]->updateEntityPosition(p->getEid(), (int16_t)x, (int16_t)y, (int16_t)z, p->isOnGround());
    }
}

void lobby::updatePlayerRotation(const player* p, float yaw, float pitch){
    for(unsigned int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL || this->players[i] == p){
            continue;
        }
        spdlog::debug("Moving player {}({}) rotation to ({}, {})", p->getUUID(), p->getIndex(), yaw, pitch);
        this->players[i]->updateEntityRotation(p->getEid(), yaw, pitch, p->isOnGround());
    }
}

void lobby::updatePlayerPositionRotation(const player* p, int x, int y, int z, float yaw, float pitch){
    for(unsigned int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL || this->players[i] == p){
            continue;
        }
        if(y <= 0){
            y = 1;
        }
        spdlog::debug("Moving player {}({}) position by ({}, {}, {}) and rotation to ({}, {})", p->getUUID(), p->getIndex(), x, y, z, yaw, pitch);
        this->players[i]->updateEntityPositionRotation(p->getEid(), (int16_t)x, (int16_t)y, (int16_t)z, yaw, pitch, p->isOnGround());
    }
}

void lobby::spawnPlayer(const player* p){
    for(unsigned int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL || this->players[i] == p){
            continue;
        }
        this->players[i]->spawnPlayer(p);
    }
}
