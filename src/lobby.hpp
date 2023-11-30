#ifndef LOBBY_HPP

#define LOBBY_HPP

#include "player.hpp"

class lobby{
    public:
        lobby(unsigned int maxPlayers);
        void addPlayer(player p);
        unsigned int getPlayerCount();
    private:
        player *players;
        unsigned int playerCount;
        const unsigned int maxPlayers;
};

#endif
