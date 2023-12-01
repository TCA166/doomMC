#ifndef PLAYER_HPP

#define PLAYER_HPP

#include "client.hpp"

class lobby;

class player : public client{
    public :
        player(server* server, int fd, state_t state, char* username, int compression, int32_t protocol);
        /*!
         @brief Handles a provided packet in reference to this instance
         @param p the packet to handle
         @return 1 if the packet was handled successfully, 0 on EOF, -1 if an error occurred
        */
        int handlePacket(packet* p);
        /*!
         @brief Sets the weapons of the player
         @param damage the damage of each weapon
         @param maxAmmo the maximum ammo of each weapon
         @param rateOfFire the rate of fire of each weapon
        */
        void setWeapons(int damage[9], int maxAmmo[9], int rateOfFire[9]);
        /*!
         @brief Sets the ammo of the player
         @param ammo the ammo of each weapon
        */
        void setAmmo(int ammo[9]);
        /*!
         @brief Sets the health of the player
         @param health the health of the player
        */
        void setHealth(int health);
        /*!
         @brief Updates the position of the player
         @param x the new absolute x position
         @param y the new absolute y position
         @param z the new absolute z position
        */
        void setLocation(double x, double y, double z);
        /*!
         @brief Deals some damage to the player
         @param damage the amount to deal
        */
        void dealDamage(int damage);
        /*!
         @brief Sends a player message to this player
         @param message the message contents
        */
        void sendMessage(char* message);
        /*!
         @brief Changes this player's lobby to the new one
         @param lobby the new lobby
        */
        void changeLobby(lobby* lobby);
        /*!
         @brief Initializes the player for play state
        */
        void startPlay(int32_t eid, lobby* assignedLobby);
        int heldSlot;
    private:
        double x, y, z;
        bool onGround;
        float yaw, pitch;
        int health;
        int damage[9];
        int ammo[9];
        int maxAmmo[9];
        int rateOfFire[9];
        lobby* currentLobby;
        int32_t eid;
};

#endif
