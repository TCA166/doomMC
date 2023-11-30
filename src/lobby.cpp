#include "lobby.hpp"
#include <stdlib.h>

lobby::lobby(unsigned int maxPlayers) : maxPlayers(maxPlayers){
    this->numPlayers = 0;
    this->players = (player*)calloc(maxPlayers, sizeof(player));
}


