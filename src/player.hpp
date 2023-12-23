#ifndef PLAYER_HPP

#define PLAYER_HPP

#include "client.hpp"
#include "weapons.hpp"

extern "C"{
    #include "C/mcTypes.h"
}

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
         @brief Sets the weapons and ammo of the player, and sends the set inventory packet
         @param weapons the weapons to set
         @param ammo the ammo to set
        */
        void setWeapons(const struct weapon* weapons, const struct ammo* ammo);
        /*!
         @brief Sets the health of the player and sends the approriate packet
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
         @brief Sends a syncrhonize location packet to the player
        */
        void synchronizeLocation();
        /*!
         @brief Deals some damage to the player and sends the appropriate packet
         @param damage the amount to deal
         @param eid the entity id of the damager
         @param damageType the type of damage to deal
        */
        void dealDamage(int damage, int32_t eid, int damageType);
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
        struct weapon* weapons;
        struct ammo* ammo;
        int currentSlot;
        lobby* currentLobby;
        int32_t eid;
        void sendChunk(palettedContainer* sections, size_t sectionCount, int chunkX, int chunkZ);
};

#endif
