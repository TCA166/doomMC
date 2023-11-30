#include "lobby.hpp"
#include <stdlib.h>

lobby::lobby(unsigned int maxPlayers) : maxPlayers(maxPlayers){
    this->playerCount = 0;
    this->players = (player**)calloc(maxPlayers, sizeof(player*));
}

unsigned int lobby::getPlayerCount(){
    return this->playerCount;
}

void lobby::addPlayer(player* p){
    if(this->playerCount >= this->maxPlayers){
        return;
    }
    for(int i = 0; i < this->playerCount; i++){
        if(this->players[i] == NULL){
            this->players[i] = p;
        }
    }
    this->playerCount++;
}
