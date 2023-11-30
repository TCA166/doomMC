#ifndef SERVER_HPP

#define SERVER_HPP

#include "lobby.hpp"
#include "client.hpp"

extern "C"{
    #include "../cJSON/cJSON.h"
}

class server{    
    private:
        const unsigned long lobbyCount;
        unsigned long connectedCount;
        const unsigned long maxConnected;
        lobby** lobbies;
        client** connected;
        const cJSON* message;
    public:
        unsigned long getPlayerCount();
        unsigned long getLobbyCount();
        unsigned long getConnectedCount();
        cJSON* getMessage();
        server(unsigned long maxPlayers, unsigned long lobbyCount, unsigned long maxConnected, cJSON* status);
        void createClient(int socket); 
        client* getClient(int n);
        void disconnectClient(int n);
};

#endif
