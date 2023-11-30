#include "player.hpp"

#ifndef LOBBY_HPP

#define LOBBY_HPP

class lobby{
    public:
        const unsigned int maxPlayers;
        lobby(unsigned int maxPlayers);
        void addPlayer(player p);
        unsigned int numPlayers;
    private:
        player *players;
};

#endif
