#ifndef LOBBY_HPP

#define LOBBY_HPP

extern "C"{
    #include <pthread.h>
    #include <sys/select.h>
    #include "mcTypes.h"
}

#include "player.hpp"
#include "weapons.hpp"

class lobby{
    public:
        lobby(unsigned int maxPlayers, const struct weapon* weapons, const struct ammo* ammo);
        lobby(unsigned int maxPlayers);
        /*!
         @brief Adds a player to the lobby, and initializes the player inside the lobby
         @param p the player to add
        */
        void addPlayer(player* p);
        /*!
         @brief Gets the player count
         @return the player count
        */
        unsigned int getPlayerCount();
        /*!
         @brief Gets the maximum number of players
         @return the maximum number of players
        */
        unsigned int getMaxPlayers();
        /*!
         @brief Sends a chat message to all players in the lobby
         @param message the message to send
        */
        void sendMessage(char* message);
        /*!
         @brief Removes a player from the lobby
         @param p the player to remove
        */
        void removePlayer(player* p);
        /*!
         @brief Gets the registry codec
         @return the registry codec
        */
        byteArray getRegistryCodec();
        /*!
         @brief Gets the weapons
         @return the weapons for this lobby instance
        */
        const struct weapon* getWeapons();
        /*!
         @brief Gets the ammo
         @return the ammo for this lobby instance
        */
        const struct ammo* getAmmo();
    private:
        pthread_t monitor;
        pthread_t main;
        player** players;
        unsigned int playerCount;
        const unsigned int maxPlayers;
        /*!
         @brief Thread function to monitor the lobby
         @param thisLobby this
        */
        static void* monitorPlayers(lobby* thisLobby);
        struct timeval monitorTimeout; // the timeval struct used in monitorPlayers
        byteArray registryCodec;
        byteArray createRegistryCodec();
        const struct weapon* weapons;
        const struct ammo* ammo;
};

#endif
