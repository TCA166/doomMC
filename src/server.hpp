#ifndef SERVER_HPP

#define SERVER_HPP

#include "lobby.hpp"
#include "client.hpp"

extern "C"{
    #include "../cJSON/cJSON.h"
}

#define NO_CONFIG 763

class server{    
    private:
        const unsigned long lobbyCount;
        unsigned long connectedCount;
        const unsigned long maxConnected;
        lobby** lobbies;
        client** connected;
        const cJSON* message;
    public:
        /*!
         @brief Gets the player count
         @return the player count
        */
        unsigned long getPlayerCount();
        /*!
         @brief Gets the lobby count
         @return the lobby count
        */
        unsigned long getLobbyCount();
        /*!
         @brief Gets the number of the connected players
         @return the connected player count
        */
        unsigned long getConnectedCount();
        /*!
         @brief Gets the status message
         @return the status message
        */
        cJSON* getMessage();
        /*!
         @brief Creates a new server instance
         @param maxPlayers the maximum number of players per lobby
         @param lobbyCount the number of lobbies
         @param maxConnected the maximum number of clients
         @param status the status message
        */
        server(unsigned long maxPlayers, unsigned long lobbyCount, unsigned long maxConnected, cJSON* status);
        /*!
         @brief Creates a new client instance
         @param socket the socket file descriptor associated with the client
        */  
        void createClient(int socket); 
        /*!
         @brief Retrieves a client
         @return Client stored under the index
        */
        client* getClient(int n);
        /*!
         @brief Disconnects a client
         @param n the index of the client to disconnect
        */
        void disconnectClient(int n);
        /*!
         @brief Adds a client to a random lobby with space
         @param c the client to add to the lobby
        */
        void addToLobby(client* c);
};

#endif
