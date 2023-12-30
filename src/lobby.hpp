#ifndef LOBBY_HPP

#define LOBBY_HPP

extern "C"{
    #include <pthread.h>
    #include <sys/select.h>
    #include "C/mcTypes.h"
}

#define sectionMax 24

#define infiniteTime -1 //infinity for epoll_wait

#include "player.hpp"
#include "weapons.hpp"
#include "map/minecraftMap.hpp"

class lobby{
    public:
        lobby(unsigned int maxPlayers, const byteArray* registryCodec, const struct weapon* weapons, const struct ammo* ammom, const minecraftMap* map);
        lobby(unsigned int maxPlayers, const byteArray* registryCodec, const minecraftMap* map);
        ~lobby();
        /*!
         @brief Adds a player to the lobby, and initializes the player inside the lobby
         @param p the player to add
        */
        void addPlayer(player* p);
        /*!
         @brief Gets the player count
         @return the player count
        */
        unsigned int getPlayerCount() const;
        /*!
         @brief Gets the maximum number of players
         @return the maximum number of players
        */
        unsigned int getMaxPlayers() const;
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
        const byteArray* getRegistryCodec() const;
        /*!
         @brief Gets the weapons
         @return the weapons for this lobby instance
        */
        const struct weapon* getWeapons() const;
        /*!
         @brief Gets the ammo
         @return the ammo for this lobby instance
        */
        const struct ammo* getAmmo() const;
        /*!
         @brief Gets the associated map
         @return the map for this lobby instance
        */
        const minecraftMap* getMap() const;
        /*!
         @brief Gets the player at the specified index
         @param n the index
         @return the player at the specified index
        */
        const player* getPlayer(int n) const;
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
        const byteArray* registryCodec;
        const struct weapon* weapons;
        const struct ammo* ammo;
        const minecraftMap* map;
        int epollFd;
        int epollPipe[2];
};

#endif
