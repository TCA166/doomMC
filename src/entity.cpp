#include "entity.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>

entity::entity(UUID_t entityUUID, int32_t eid, entity_t entityType, int32_t entityData) : entityUUID(entityUUID){
    this->eid = eid;
    this->entityType = entityType;
    this->entityData = entityData;
}

entity::entity(UUID_t entityUUID, int32_t eid, entity_t entityType, int32_t entityData, double x, double y, double z, float yaw, float pitch, float headYaw) : entity(entityUUID, eid, entityType, entityData){
    this->x = x;
    this->y = y;
    this->z = z;
    this->yaw = yaw;
    this->pitch = pitch;
    this->headYaw = headYaw;
}

int32_t entity::getEid() const{
    return this->eid;
}

int entity::getBlock(int x, int y, int z) const{
    int block = 0;
    try{
        block = this->currentLobby->getMap()->getBlock((uint32_t)(this->x + x), (uint32_t)(this->y + y), (uint32_t)(this->z + z));
    }
    catch(const std::invalid_argument& e){
        spdlog::error("Could not get block at {},{},{}: {}", (uint32_t)(this->x + x), (uint32_t)(this->y + y), (uint32_t)(this->z + z), e.what());
    }
    return block;
}
