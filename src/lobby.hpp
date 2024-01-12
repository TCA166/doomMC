#ifndef LOBBY_HPP

#define LOBBY_HPP

extern "C"{
    #include <pthread.h>
    #include <sys/select.h>
    #include "C/mcTypes.h"
}

#define sectionMax 24

#define infiniteTime -1 //infinity for epoll_wait

typedef void*(*thread)(void*);

class player;
#include "weapons.hpp"
#include "map/map.hpp"

class lobby{
    public:
        lobby(unsigned int maxPlayers, const byteArray* registryCodec, const struct weapon* weapons, const struct ammo* ammom, const map* lobbyMap);
        lobby(unsigned int maxPlayers, const byteArray* registryCodec, const map* lobbyMap);
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
        const map* getMap() const;
        /*!
         @brief Gets the player at the specified index
         @param n the index
         @return the player at the specified index
        */
        const player* getPlayer(int n) const;
        /*!
         @brief Disconnects a player from the lobby
         @param n the index of the player to disconnect
        */
        void disconnectPlayer(int n);
        /*!
         @brief Updates the player's position on other clients
         @param eid the entity id of the player
         @param x x delta
         @param y y delta
         @param z z delta
        */
        void updatePlayerPosition(const player* p, int x, int y, int z);
        /*!
         @brief Updates the player's rotation on other clients
         @param eid the entity id of the player
         @param yaw the yaw delta
         @param pitch the pitch delta
        */
        void updatePlayerRotation(const player* p, float yaw, float pitch);
        /*!
         @brief Updates the player's position and rotation on other clients
         @param eid the entity id of the player
         @param x x delta
         @param y y delta
         @param z z delta
         @param yaw the yaw delta
         @param pitch the pitch delta
        */
        void updatePlayerPositionRotation(const player* p, int x, int y, int z, float yaw, float pitch);
        /*!
         @brief Spawns the player for all the other players in lobby
         @param p the player to sync
        */
        void spawnPlayer(const player* p);
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
        /*!
         @brief Thread function to run the lobby
         @param thisLobby this
        */
        static void* mainLoop(lobby* thisLobby);
        const byteArray* registryCodec;
        const struct weapon* weapons;
        const struct ammo* ammo;
        const map* lobbyMap;
        int epollFd;
        int epollPipe[2];
};

#endif
