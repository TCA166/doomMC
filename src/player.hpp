#ifndef PLAYER_HPP

#define PLAYER_HPP

#include "client.hpp"
#include "weapons.hpp"
#include "entity.hpp"

extern "C"{
    #include "C/mcTypes.h"
    #include <time.h>
}

class player : public client, public entity{
    public :
        player(server* server, int fd, state_t state, char* username, int compression, int32_t protocol, UUID_t uuid);
        virtual ~player();
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
        /*!
         @brief Disconnects the player
        */
        virtual void disconnect();
        int heldSlot;
        /*!
         @brief Sends a keep alive packet to the player
        */
        void keepAlive();
        /*!
         @brief Gets the player's ping
         @return the time difference between the last keep alive and now
        */
        time_t getPing() const;
        /*!
         @brief Updates a position of an entity for this player
         @param eid the entity id of the entity
         @param x the delta x
         @param y the delta y
         @param z the delta z
         @param onGround whether or not the entity is on the ground
        */
        void updateEntityPosition(int32_t eid, int16_t x, int16_t y, int16_t z, bool onGround);
        /*!
         @brief Updates the rotation of an entity for this player
         @param eid the entity id of the entity
         @param yaw the yaw of the entity
         @param pitch the pitch of the entity
         @param onGround whether or not the entity is on the ground
        */
        void updateEntityRotation(int32_t eid, float yaw, float pitch, bool onGround);
        /*!
         @brief Updates the position and rotation of an entity for this player
         @param eid the entity id of the entity
         @param x the delta x
         @param y the delta y
         @param z the delta z
         @param yaw the yaw of the entity
         @param pitch the pitch of the entity
         @param onGround whether or not the entity is on the ground
        */
        void updateEntityPositionRotation(int32_t eid, int16_t x, int16_t y, int16_t z, float yaw, float pitch, bool onGround);
        /*!
         @brief Spawns an entity for the player
         @param ent entity to spawn
        */
        void spawnEntity(const entity* ent);
        /*!
         @brief Spawns a player for this player
         @param p the player to spawn
        */
        void spawnPlayer(const player* p);
        void removePlayer(const player* p);
        void removeEntity(const entity* ent);
        /*!
         @brief Gets if the player is currently on the ground
         @return whether or not the player is on the ground
        */
        bool isOnGround() const;
        UUID_t getUUID() const;
    private:
        bool onGround;
        int health;
        struct weapon* weapons;
        struct ammo* ammo;
        int currentSlot;
        void sendChunk(palettedContainer* sections, size_t sectionCount, int chunkX, int chunkZ);
        void setCenterChunk(int chunkX, int chunkZ);
        unsigned long teleportId; //last teleport id
        time_t lastKeepAlive;
};

#endif
