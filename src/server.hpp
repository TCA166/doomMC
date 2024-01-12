#ifndef SERVER_HPP

#define SERVER_HPP

#include "lobby.hpp"
#include "client.hpp"

extern "C"{
    #include "../cJSON/cJSON.h"
    #include <netinet/in.h>
}

#define NO_CONFIG 763

class server{    
    private:
        const unsigned int lobbyCount;
        unsigned int connectedCount;
        const unsigned int maxConnected;
        //array of lobbies in server
        lobby** lobbies;
        //array of connected clients
        client** connected;
        //the status message
        cJSON* message;
        //stored registry codec
        byteArray registryCodec;
        //epoll file descriptor
        int epollFd;
        //master socket for receiving new connections
        int masterSocket;
        struct sockaddr_in address;
        size_t addrLen;
    public:
        /*!
         @brief Gets the player count
         @return the player count
        */
        unsigned int getPlayerCount();
        /*!
         @brief Gets the lobby count
         @return the lobby count
        */
        unsigned int getLobbyCount();
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
         @param registryCodec parsed template registry codec
        */
        server(uint16_t port, unsigned int maxPlayers, unsigned int lobbyCount, unsigned int maxConnected, const char* statusFilename, const char* registryCodecFilename, const char* versionFilename);
        virtual ~server();
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
         @brief Removes a client
         @param n the index of the client to remove
        */
        void removeClient(int n);
        /*!
         @brief Adds a client to a random lobby with space
         @param c the client to add to the lobby
        */
        void addToLobby(client* c);
        /*!
         @brief Gets the registry codec
         @return the registry codec
        */
        const byteArray* getRegistryCodec();
        /*!
         @brief Runs the server 
         @return 0 on success, -1 on error
        */
        int run();
};

#endif
