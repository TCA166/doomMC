#ifndef ENTITY_HPP

#define ENTITY_HPP

extern "C"{
    #include "C/mcTypes.h"
}

#include "lobby.hpp"

typedef enum{
    ENTITY_EXPERIENCE_ORB=34,
    ENTITY_TYPE_ITEM=54,
    ENTITY_TYPE_PLAYER=122
} entity_t;


class entity{
    protected:
        int32_t eid; //the entity id
        double x, y, z;
        float yaw, pitch, headYaw;
        lobby* currentLobby;
    private:
        const UUID_t entityUUID;
        entity_t entityType;
        int32_t entityData;
    public:
        entity(UUID_t entityUUID, int32_t eid, entity_t entityType, int32_t entityData);
        entity(UUID_t entityUUID, int32_t eid, entity_t entityType, int32_t entityData, double x, double y, double z, float yaw, float pitch, float headYaw);
        /*!
         @brief Gets the entity id of this player
         @return the entity id of this player
        */
        int32_t getEid() const;
        /*!
         @brief Get's the block at the specified offset from the player position
         @param x the x offset
         @param y the y offset
         @param z the z offset
         @return the block at the specified offset
        */
        int getBlock(int x, int y, int z) const;
};

#endif
