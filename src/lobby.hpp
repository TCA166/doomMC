#ifndef LOBBY_HPP

#define LOBBY_HPP
#include <pthread.h>
#include <sys/select.h>

class player;

struct weaponSettings{
    int damage;
    int maxAmmo;
    int rateOfFire;
};

class lobby{
    public:
        lobby(unsigned int maxPlayers);
        void addPlayer(player* p);
        unsigned int getPlayerCount();
        unsigned int getMaxPlayers();
        void sendMessage(char* message);
        void removePlayer(player* p);
    private:
        pthread_t monitor;
        pthread_t main;
        player** players;
        unsigned int playerCount;
        const unsigned int maxPlayers;
        static void* monitorPlayers(lobby* thisLobby);
        struct timeval monitorTimeout;
};

#endif
