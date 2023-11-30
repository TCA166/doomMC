#include "lobby.hpp"
#include <stdlib.h>

lobby::lobby(unsigned int maxPlayers) : maxPlayers(maxPlayers){
    this->playerCount = 0;
    this->players = (player*)calloc(maxPlayers, sizeof(player));
}

unsigned int lobby::getPlayerCount(){
    return this->playerCount;
}

