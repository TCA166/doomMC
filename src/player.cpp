#include "player.hpp"
#include "lobby.hpp"
#include "server.hpp"

extern "C"{
    #include <string.h>
}

player::player(server* server, int fd, state_t state, char* username, int compression, int32_t protocol) : client(server, fd, state, username, compression, protocol){
    //initialize the not inherited fields
    this->currentLobby = NULL;
    this->eid = 0;
    this->heldSlot = 0;
    this->x = 0;
    this->y = 0;
    this->z = 0;
    this->yaw = 0;
    this->pitch = 0;
    this->onGround = false;
    this->health = 20;
}

void player::setWeapons(const struct weapon* weapons, const struct ammo* ammo){
    this->weapons = (struct weapon*)weapons;
    this->ammo = (struct ammo*)ammo;
    this->currentSlot = 0;
    byte data[1 + (2 * MAX_VAR_INT) + (sizeof(slot) * 45)];
    size_t offset = 0;
    data[0] = 0; //window id
    offset++;
    offset += writeVarInt(data + offset, 0); //state id
    offset += writeVarInt(data + offset, 45); //slot count
    slot slots[45];
    memset(slots, 0, sizeof(slot) * 45);
    for(int i = 0; i < 36; i++){
        offset += writeSlot(data + offset, &slots[i]);
    }
    for(int i = 36; i < 45; i++){
        struct weapon* weapon = &this->weapons[i - 36];
        slots[i].present = weapon->owned;
        slots[i].id = 1;
        slots[i].count = this->ammo[weapon->ammoId].count;
        offset += writeSlot(data + offset, &slots[i]);
    }
    offset += writeSlot(data + offset, &slots[36 + this->currentSlot]);
    this->send(data, offset, SET_CONTAINER_CONTENT);
}

void player::setHealth(int health){
    this->health = health;
    byte data[(sizeof(float) * 2) + MAX_VAR_INT];
    size_t offset = 0;
    offset += writeBigEndianFloat(data, (float)this->health);
    offset += writeVarInt(data + offset, 20);
    offset += writeBigEndianFloat(data + offset, 0);
    this->send(data, offset, SET_HEALTH);
}

void player::setLocation(double x, double y, double z){
    this->x = x;
    this->y = y;
    this->z = z;
    byte data[(sizeof(double) * 3) + (sizeof(float) * 2) + MAX_VAR_INT];
    size_t offset = 0;
    offset += writeBigEndianDouble(data + offset, this->x);
    offset += writeBigEndianDouble(data + offset, this->y);
    offset += writeBigEndianDouble(data + offset, this->z);
    offset += writeBigEndianFloat(data + offset, this->yaw);
    offset += writeBigEndianFloat(data + offset, this->pitch);
    data[offset] = 0;
    offset++;
    offset += writeVarInt(data + offset, 0);
    this->send(data, offset, SYNCHRONIZE_PLAYER_POSITION);
}

void player::dealDamage(int damage, int32_t eid, int damageType){
    this->health -= damage;
    byte data[1 + (MAX_VAR_INT * 5)];
    size_t offset = 0;
    offset += writeVarInt(data, this->eid);
    offset += writeVarInt(data, damageType);
    offset += writeVarInt(data + offset, eid);
    offset += writeVarInt(data + offset, eid);
    data[offset] = false;
    offset++;
    this->send(data, offset, DAMAGE_EVENT);
}

void player::sendMessage(char* message){
    size_t msgLen = strlen(message);
    byte packet[sizeof(UUID_t) + (MAX_VAR_INT * 2) + 1 + msgLen];
    *((UUID_t*)packet) = (UUID_t)0;
    size_t sz1 = writeVarInt(packet + sizeof(UUID_t), 0);
    *(byte*)(packet + sizeof(UUID_t) + sz1) = 0;
    size_t sz2 = writeString(packet + sizeof(UUID_t) + sz1 + 1, message, msgLen);
    this->send(packet, sizeof(UUID_t) + sz1 + 1 + sz2, PLAYER_CHAT_MESSAGE);
}

void player::changeLobby(lobby* lobby){
    if(this->currentLobby != NULL){
        this->currentLobby->removePlayer(this);
    }
    lobby->addPlayer(this);
    this->currentLobby = lobby;
}

void player::startPlay(int32_t eid, lobby* assignedLobby){
    if(this->currentLobby != NULL){
        return;
    }
    this->currentLobby = assignedLobby;
    this->eid = eid++;
    { //send LOGIN_PLAY
        char* dimensionName = "minecraft:overworld";
        byteArray registryCodec = this->currentLobby->getRegistryCodec();
        byte data[(sizeof(int32_t) * 2) + 10 + (20 * 3) + (MAX_VAR_INT * 5) + registryCodec.len];
        size_t offset = writeBigEndianInt(data, eid);
        data[offset] = false; //not hardcore
        offset++;
        data[offset] = 0; //gamemode
        offset++;
        data[offset] = -1; //prev gamemode
        offset++;
        offset += writeVarInt(data + 7, 1);
        offset += writeString(data + offset, dimensionName, 19);
        //write the registry codec
        memcpy(data + offset, registryCodec.bytes, registryCodec.len);
        offset += registryCodec.len;
        offset += writeString(data + offset, dimensionName, 19);
        offset += writeString(data + offset, dimensionName, 19);
        offset += writeBigEndianLong(data + offset, 0); //seed
        offset += writeVarInt(data + offset, this->currentLobby->getMaxPlayers()); //max players
        offset += writeVarInt(data + offset, 8); //draw distance
        offset += writeVarInt(data + offset, 8); //sim distance
        data[offset] = false; //reduced debug info
        offset++;
        data[offset] = false; //immediate respawn
        offset++;
        data[offset] = true; //debug
        offset++;
        data[offset] = false; //flat
        offset++;
        data[offset] = false; //death location
        offset++;
        offset += writeVarInt(data + offset, 0); //portal cooldown
        int res = this->send(data, offset - 1, LOGIN_PLAY); //TODO figure out why the -1 is needed
    }
    //https://wiki.vg/Protocol_FAQ#I_think_I.27ve_done_everything_right.2C_but.E2.80.A6
    //send chunk data and update light
    {
        //TODO
    }
    //send set default spawn position
    {
        byte data[sizeof(position) + sizeof(float)];
        size_t offset = 0;
        position p = toPosition(0, 0, 0);
        offset += writeBigEndianLong(data + offset, p);
        //angle
        offset += writeBigEndianFloat(data + offset, 0);
        this->send(data, offset, SET_DEFAULT_SPAWN_POSITION);
    }
    //then send Set Container Content
    this->setWeapons(this->currentLobby->getWeapons(), this->currentLobby->getAmmo());
    this->setHealth(20);
    this->setLocation(0, 0, 0);
}

int player::handlePacket(packet* p){
    if(this->state != PLAY_STATE){
        return -1;
    }
    int offset = 0;
    switch(p->packetId){
        case CHAT_MESSAGE:{
            char* message = readString(p->data, &offset);
            
            break;
        }
        case SET_PLAYER_POSITION:{
            this->x = readDouble(p->data, &offset);
            this->y = readDouble(p->data, &offset);
            this->z = readDouble(p->data, &offset);
            this->onGround = readBool(p->data, &offset);
            break;
        }
        case SET_PLAYER_POSITION_AND_ROTATION:{
            this->x = readDouble(p->data, &offset);
            this->y = readDouble(p->data, &offset);
            this->z = readDouble(p->data, &offset);
            this->yaw = readFloat(p->data, &offset);
            this->pitch = readFloat(p->data, &offset);
            this->onGround = readBool(p->data, &offset);
            break;
        }
        case SET_PLAYER_ROTATION:{
            this->yaw = readFloat(p->data, &offset);
            this->pitch = readFloat(p->data, &offset);
            this->onGround = readBool(p->data, &offset);
            break;
        }
        case SET_PLAYER_ON_GROUND:{
            this->onGround = readBool(p->data, &offset);
            break;
        }
        case PICK_ITEM:{
            this->heldSlot = readVarInt(p->data, &offset);
            break;
        }
        case PLAYER_INPUT:{
            float sideways = readFloat(p->data, &offset);
            this->z += sideways;
            float forward = readFloat(p->data, &offset);
            this->x += forward;
            byte flags = readByte(p->data, &offset);
            break;
        }
        
    }
    return 1;
}