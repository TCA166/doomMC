#include "player.hpp"
#include "lobby.hpp"
#include "server.hpp"
#include <spdlog/spdlog.h>

extern "C"{
    #include <string.h>
    #include <stdlib.h>
    #include <math.h>
    #include <time.h>
}

#define timeout 2000

//https://wiki.vg/index.php?title=Protocol&oldid=18242 (1.19.4)

player::player(server* server, int fd, state_t state, char* username, int compression, int32_t protocol, UUID_t uuid) : client(server, fd, state, username, compression, protocol), entity(uuid, 0, ENTITY_TYPE_PLAYER, 0){
    //initialize the not inherited fields
    this->currentLobby = NULL;
    this->heldSlot = 0;
    this->onGround = false;
    this->health = 20;
    this->teleportId = 0;
    this->lastKeepAlive = time(NULL);
    this->hasSpawned = false;
}

void player::setWeapons(const struct weapon* weapons, const struct ammo* ammo){
    this->weapons = (struct weapon*)weapons;
    this->ammo = (struct ammo*)ammo;
    this->currentSlot = 0;
    byte data[1 + (2 * MAX_VAR_INT) + (sizeof(slot) * 45)];
    size_t offset = 0;
    data[offset++] = 0; //window id
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
    size_t offset = writeBigEndianFloat(data, (float)this->health);
    offset += writeVarInt(data + offset, 20);
    offset += writeBigEndianFloat(data + offset, 0);
    this->send(data, offset, SET_HEALTH);
}

void player::setCenterChunk(int32_t x, int32_t z){
    spdlog::debug("Setting center chunk of player {} to {}, {}", this->username, x, z);
    byte data[MAX_VAR_INT * 2];
    size_t offset = writeVarInt(data, x);
    offset += writeVarInt(data + offset, z);
    this->send(data, offset, SET_CENTER_CHUNK);
}

void player::setLocation(double x, double y, double z){
    spdlog::debug("Player {}({}) teleported to {},{},{}", this->username, this->index, x, y, z);
    this->x = x;
    this->y = y;
    this->z = z;
    this->setCenterChunk((int32_t)floor(x / 16), (int32_t)floor(z / 16));
    byte data[(sizeof(double) * 3) + (sizeof(float) * 2) + MAX_VAR_INT];
    size_t offset = writeBigEndianDouble(data, this->x);
    offset += writeBigEndianDouble(data + offset, this->y);
    offset += writeBigEndianDouble(data + offset, this->z);
    offset += writeBigEndianFloat(data + offset, this->yaw);
    offset += writeBigEndianFloat(data + offset, this->pitch);
    data[offset++] = 0;
    offset += writeVarInt(data + offset, this->teleportId++);
    this->send(data, offset, SYNCHRONIZE_PLAYER_POSITION);
}

void player::dealDamage(int damage, int32_t eid, int damageType){
    this->health -= damage;
    byte data[1 + (MAX_VAR_INT * 5)];
    size_t offset = writeVarInt(data, this->eid);
    offset += writeVarInt(data, damageType);
    offset += writeVarInt(data + offset, eid);
    offset += writeVarInt(data + offset, eid);
    data[offset++] = false;
    this->send(data, offset, DAMAGE_EVENT);
}

void player::sendMessage(char* message){
    size_t msgLen = strlen(message);
    byte* packet = new byte[msgLen + 3];
    size_t offset = 0;
    packet[offset] = TAG_STRING;
    offset += writeBigEndianShort(packet, msgLen);
    memcpy(packet + offset, message, msgLen);
    offset += msgLen;
    packet[offset++] = 0;
    this->send(packet, offset, SYSTEM_CHAT_MESSAGE);
    delete[] packet;
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
    const char* dimensionName = (const char*)"minecraft:overworld";
    this->currentLobby = assignedLobby;
    this->eid = eid;
    { //send LOGIN_PLAY
        if(this->protocol <= NO_CONFIG){
            {
                const byteArray* registryCodec = this->currentLobby->getRegistryCodec();
                byte* data = new byte[(sizeof(int32_t) * 2) + 10 + (20 * 3) + (MAX_VAR_INT * 5) + registryCodec->len];
                size_t offset = writeBigEndianInt(data, eid);
                data[offset++] = false; //not hardcore
                data[offset++] = 0; //gamemode
                data[offset++] = -1; //prev gamemode
                offset += writeVarInt(data + 7, 1);
                offset += writeString(data + offset, dimensionName, 19);
                //write the registry codec
                memcpy(data + offset, registryCodec->bytes, registryCodec->len);
                offset += registryCodec->len;
                offset += writeString(data + offset, dimensionName, 19);
                offset += writeString(data + offset, dimensionName, 19);
                offset += writeBigEndianLong(data + offset, 0); //seed
                offset += writeVarInt(data + offset, this->currentLobby->getMaxPlayers()); //max players
                offset += writeVarInt(data + offset, 8); //draw distance
                offset += writeVarInt(data + offset, 8); //sim distance
                data[offset++] = false; //reduced debug info
                data[offset++] = false; //immediate respawn
                data[offset++] = false; //debug
                data[offset++] = false; //flat
                data[offset++] = false; //death location
                offset += writeVarInt(data + offset, 0); //portal cooldown
                this->send(data, offset - 1, LOGIN_PLAY); //TODO figure out why the -1 is needed
                delete[] data;
            }
        }
        else{//TODO implement new LOGIN_PLAY format

        }
    }
    {//change difficulty
        byte data[2];
        data[0] = 3; //difficulty
        data[1] = true; //is locked
        this->send(data, 2, CHANGE_DIFFICULTY);
    }
    {//player abilities
        byte data[(sizeof(float) * 2) + 1];
        data[0] = 0x00; //flags
        size_t offset = 1;
        offset += writeBigEndianFloat(data + offset, 0.05f); //flying speed
        offset += writeBigEndianFloat(data + offset, 0.1f); //walking speed
        this->send(data, offset, PLAYER_ABILITIES);
    }
    //https://wiki.vg/Protocol_FAQ#What.27s_the_normal_login_sequence_for_a_client.3F <- this order here seems outdated to say the least
    {//send set held item
        byte data[1];
        data[0] = this->heldSlot;
        this->send(data, 1, SET_HELD_ITEM);
    }
    {//send update recipies
        byte data[MAX_VAR_INT];
        size_t offset = writeVarInt(data, 0);
        this->send(data, offset, UPDATE_RECIPES);
    }
    if(this->protocol <= NO_CONFIG){
        this->sendFeatureFlags();
        this->sendTags();
    }
    //set op permisssion level
    {
        byte data[sizeof(int32_t) + 1];
        size_t offset = writeBigEndianInt(data, this->eid);
        data[offset++] = 24;
        this->send(data, offset, ENTITY_EVENT);
    }
    //send commands
    {
        byte data[MAX_VAR_INT * 2];
        size_t offset = writeVarInt(data, 0);
        offset += writeVarInt(data + offset, 0);
        this->send(data, offset, COMMANDS);
    }
    {//server data
        byte data[21 + MAX_VAR_INT];
        const char* MOTD = "{\"text\":\"\"}";
        size_t offset = writeString(data, MOTD, 11);
        data[offset++] = false;
        data[offset++] = false;
        this->send(data, offset, SERVER_DATA);
    }
    {//update player list
        byte flag = 0x01 | 0x04 | 0x08 | 0x10;
        unsigned int playerNum = this->currentLobby->getPlayerCount();
        byte* data = new byte[1 + MAX_VAR_INT + (sizeof(UUID_t) + (4 * MAX_VAR_INT) + 17) * playerNum];
        size_t offset = 0;
        data[offset++] = flag;
        offset += writeVarInt(data + offset, playerNum);
        for(unsigned int i = 0; i < playerNum; i++){
            const player* p = this->currentLobby->getPlayer(i);
            *(data + offset) = p->getUUID();
            offset += sizeof(UUID_t);
            offset += writeString(data + offset, p->getUsername(), strlen(p->getUsername()));
            offset += writeVarInt(data + offset, 0);
            offset += writeVarInt(data + offset, 0);
            data[offset++] = true;
            offset += writeVarInt(data + offset, p->getPing());
        }
        this->send(data, offset, PLAYER_INFO_UPDATE);
        delete[] data;
    }
    const map* m = this->currentLobby->getMap();
    position spawn = m->getSpawn();
    {//send set default spawn position
        spdlog::debug("Sending set default spawn position to {},{},{} to player {}({})", positionX(spawn), positionY(spawn), positionZ(spawn), this->username, this->index);
        byte data[sizeof(position) + sizeof(float)];
        size_t offset = writeBigEndianLong(data, spawn);
        //angle
        offset += writeBigEndianFloat(data + offset, 0);
        this->send(data, offset, SET_DEFAULT_SPAWN_POSITION);
    }
    this->setCenterChunk((int32_t)floor(positionX(spawn) / 16), (int32_t)floor(positionZ(spawn) / 16));
    //send chunk data and update light
    {
        this->send(NULL, 0, BUNDLE_DELIMITER);
        //get width in chunks
        int chunkWidth = m->getWidth() / 16;
        //get length in chunks
        int chunkLength = m->getLength() / 16;
        palettedContainer sections[sectionMax];
        for(int chunkX = 0; chunkX < chunkWidth; chunkX++){
            for(int chunkZ = 0; chunkZ < chunkLength; chunkZ++){
                for(int i = 0; i < sectionMax; i++){
                    sections[i] = m->getSection(chunkX, chunkZ, i);
                }
                this->sendChunk(sections, sectionMax, chunkX, chunkZ);
                for(int i = 0; i < sectionMax; i++){
                    if(sections[i].states != NULL){
                        free(sections[i].states);
                    }
                    else{
                        free(sections[i].palette);
                    }   
                }
            }
        }
        this->send(NULL, 0, BUNDLE_DELIMITER);
    }
    {//Respawn
        byte* p = new byte[(19 * 2) + (MAX_VAR_INT * 2) + sizeof(int64_t) + 6];
        size_t offset = writeString(p, dimensionName, 19);
        offset += writeString(p + offset, dimensionName, 19);
        offset += writeBigEndianLong(p + offset, 0);
        p[offset++] = 0; //gamemode
        p[offset++] = -1; //prev gamemode
        p[offset++] = false; //is debug
        p[offset++] = false; //is flat
        p[offset++] = 0; //data kept
        p[offset++] = false; //has death location
        this->send(p, offset, RESPAWN);
        delete[] p;
    }
    {//send initialize world border
        byte data[(sizeof(double) * 4) + (MAX_VAR_INT * 4)];
        size_t offset = writeBigEndianDouble(data, 0);
        offset += writeBigEndianDouble(data + offset, 0);
        offset += writeBigEndianDouble(data + offset, 0);
        offset += writeBigEndianDouble(data + offset, 1000);
        offset += writeVarInt(data + offset, 0); //TODO should be varLong
        offset += writeVarInt(data + offset, 0);
        offset += writeVarInt(data + offset, 0);
        offset += writeVarInt(data + offset, 0);
        this->send(data, offset, INITIALIZE_WORLD_BORDER);
    }
    //then send Set Container Content
    this->setWeapons(this->currentLobby->getWeapons(), this->currentLobby->getAmmo());
    this->setHealth(20);
    this->setLocation(positionX(spawn), positionY(spawn), positionZ(spawn));
    this->currentLobby->spawnPlayer(this);
    this->hasSpawned = true;
    spdlog::info("Player {} joined lobby", this->username);
}

static inline bool verifyOnGround(player* p){
    return p->getBlock(0, -1, 0) != 0;
}

int player::handlePacket(packet* p){
    //TODO sync and expand
    if(this->state != PLAY_STATE){
        return -1;
    }
    if(!this->hasSpawned){
        return 1;
    }
    int offset = 0;
    switch(p->packetId){
        case CHAT_MESSAGE:{
            char* message = readString(p->data, &offset);
            spdlog::info("Player {}({}) sent message: {}", this->username, this->index, message);
            //TODO handle
            break;
        }
        case SET_PLAYER_POSITION:{
            double x = readBigEndianDouble(p->data, &offset);
            double y = readBigEndianDouble(p->data, &offset);
            double z = readBigEndianDouble(p->data, &offset);
            bool onGround = readBool(p->data, &offset);
            if(verifyOnGround(this) == onGround){
                this->onGround = onGround;
            }
            if(this->x != x || this->y != y || this->z != z){
                this->currentLobby->updatePlayerPosition(this, (int32_t)(this->x - x), (int32_t)(this->y - y), (int32_t)(this->z - z));
                this->x = x;
                this->y = y;
                this->z = z;
            }
            break;
        }
        case SET_PLAYER_POSITION_AND_ROTATION:{
            double x = readBigEndianDouble(p->data, &offset);
            double y = readBigEndianDouble(p->data, &offset);
            double z = readBigEndianDouble(p->data, &offset);
            this->yaw = readBigEndianFloat(p->data, &offset);
            this->pitch = readBigEndianFloat(p->data, &offset);
            bool onGround = readBool(p->data, &offset);
            if(verifyOnGround(this) == onGround){
                this->onGround = onGround;
            }
            if(this->x != x || this->y != y || this->z != z){
                this->currentLobby->updatePlayerPositionRotation(this, (int32_t)(this->x - x), (int32_t)(this->y - y), (int32_t)(this->z - z), this->yaw, this->pitch);
                this->x = x;
                this->y = y;
                this->z = z;
            }
            break;
        }
        case SET_PLAYER_ROTATION:{
            this->yaw = readBigEndianFloat(p->data, &offset);
            this->pitch = readBigEndianFloat(p->data, &offset);
            bool onGround = readBool(p->data, &offset);
            if(verifyOnGround(this) == onGround){
                this->onGround = onGround;
            }
            this->currentLobby->updatePlayerRotation(this, this->yaw, this->pitch);
            break;
        }
        case SET_PLAYER_ON_GROUND:{
            bool onGround = readBool(p->data, &offset);
            if(verifyOnGround(this) == onGround){
                this->onGround = onGround;
            }
            break;
        }
        case PICK_ITEM:{
            this->heldSlot = readVarInt(p->data, &offset);
            break;
        }
        case PLAYER_INPUT:{
            float sideways = readBigEndianFloat(p->data, &offset);
            float forward = readBigEndianFloat(p->data, &offset);
            byte jump = readByte(p->data, &offset);
            jump = jump & 0x1;
            this->currentLobby->updatePlayerPosition(this, (int32_t)forward, 0, (int32_t)sideways);
            this->z += sideways;
            this->x += forward;
            break;
        }
        case KEEP_ALIVE_2:{
            this->lastKeepAlive = (time_t)readBigEndianLong(p->data, &offset);
            //if the ping is greater than 20 s
            if(this->getPing() > 20){
                this->disconnect();
                this->currentLobby->removePlayer(this);
                delete this;
            }
            break;
        }
    }
    return 1;
}

void player::sendChunk(palettedContainer* sections, size_t sectionCount, int chunkX, int chunkZ){
    const size_t neededLater = ((MAX_VAR_INT + sizeof(int64_t)) * 4) + (MAX_VAR_INT * 2) + ((MAX_VAR_INT + ((MAX_VAR_INT + 2048) * sectionCount)) * 2);
    byte* data = (byte*)malloc((sizeof(int32_t) * 2) + (sizeof(int16_t) * 2) + (17));
    size_t offset = writeBigEndianInt(data, chunkX);
    offset += writeBigEndianInt(data + offset, chunkZ);
    { //heightmap NBT
        //construct the heightmap
        int32_t* heightmap = new int32_t[256];
        for(int x = 0; x < 16; x++){
            for(int z = 0; z < 16; z++){
                bool found = false;
                //foreach section
                for(int s = sectionCount - 1; s >= 0; s--){
                    palettedContainer* section = &sections[s];
                    if(section->states == NULL){
                        continue;
                    }
                    //foreach y in section
                    for(int y = 15; y >= 0; y--){
                        int index = statesFormula(x, y, z);
                        if(section->states[index] != 0){
                            found = true;
                            heightmap[(x * 16) + z] = y;
                            break;
                        }
                    }
                    if(found){
                        break;
                    }
                }
            }
        }
        //then write it into a NBT
        data[offset++] = TAG_COMPOUND;
        offset += writeBigEndianShort(data + offset, 0);
        data[offset++] = TAG_LONG_ARRAY;
        offset += writeBigEndianShort(data + offset, 15);
        memcpy(data + offset, "MOTION_BLOCKING", 15);
        offset += 15;
        const uint8_t bpe = ceil(log2((sectionCount * 16) + 1));
        byteArray heightmapNBT = writePackedArray(heightmap, 256, bpe, false);
        delete[] heightmap;
        data = (byte*)realloc(data, offset + heightmapNBT.len + 2 + MAX_VAR_INT + sizeof(int32_t));
        offset += writeBigEndianInt(data + offset, heightmapNBT.len/sizeof(int64_t));
        memcpy(data + offset, heightmapNBT.bytes, heightmapNBT.len);
        free(heightmapNBT.bytes);
        offset += heightmapNBT.len;
        data[offset++] = TAG_INVALID;
    }
    byteArray buff = writeSections(sections, NULL, sectionCount, 0);
    offset += writeVarInt(data + offset, (int32_t)buff.len);
    data = (byte*)realloc(data, offset + buff.len + neededLater);
    memcpy(data + offset, buff.bytes, buff.len);
    offset += buff.len;
    free(buff.bytes);
    //block entities
    offset += writeVarInt(data + offset, 0);
    //Trust edges
    data[offset++] = false;
    {//bit sets for sky light
        if(sectionCount > sizeof(int64_t) * 8){
            spdlog::error("Too many sections for sky light bit set");
            return;
        }
        int64_t zero = 0;
        int64_t one = 0xFFFFFFFFFFFFFFFF; //16 times F
        const bitSet empty = {1, &zero};
        const bitSet full = {1, &one};
        offset += writeBitSet(data + offset, &empty);
        offset += writeBitSet(data + offset, &empty);
        offset += writeBitSet(data + offset, &full);
        offset += writeBitSet(data + offset, &full);
        offset += writeVarInt(data + offset, 0); //sky light array length
        offset += writeVarInt(data + offset, 0);
    }
    this->send(data, offset, CHUNK_DATA_AND_UPDATE_LIGHT);
    free(data);
}

void player::disconnect(){
    if(this->fd == -1){
        return;
    }
    spdlog::debug("Disconnecting client {}({})", this->getUUID(), this->index);
    this->send(NULL, 0, DISCONNECT_PLAY); //send disconnect packet
    this->currentLobby->removePlayer(this);
}

player::~player(){
    //FIXME invalid pointer exception after this was called when two players were on the server
    spdlog::debug("Deleting player {}({})", this->getUUID(), this->index);
}

void player::keepAlive(){
    byte data[sizeof(int64_t)];
    size_t offset = writeBigEndianLong(data, (int64_t)time(NULL));
    this->send(data, offset, KEEP_ALIVE);
}

time_t player::getPing() const{
    return time(NULL) - this->lastKeepAlive;
}

void player::updateEntityPosition(int32_t eid, int16_t x, int16_t y, int16_t z, bool onGround){
    byte data[MAX_VAR_INT + sizeof(int16_t) * 3 + 1];
    size_t offset = writeVarInt(data, eid);
    offset += writeBigEndianShort(data + offset, x);
    offset += writeBigEndianShort(data + offset, y);
    offset += writeBigEndianShort(data + offset, z);
    data[offset++] = onGround;
    this->send(data, offset, UPDATE_ENTITY_POSITION);
}

void player::updateEntityRotation(int32_t eid, float yaw, float pitch, bool onGround){
    byte data[MAX_VAR_INT + sizeof(float) * 2 + 1];
    size_t offset = writeVarInt(data, eid);
    data[offset++] = toAngle(yaw);
    data[offset++] = toAngle(pitch);
    data[offset++] = onGround;
    this->send(data, offset, UPDATE_ENTITY_ROTATION);
}

void player::updateEntityPositionRotation(int32_t eid, int16_t x, int16_t y, int16_t z, float yaw, float pitch, bool onGround){
    byte data[MAX_VAR_INT + sizeof(int16_t) * 3 + sizeof(float) * 2 + 1];
    size_t offset = writeVarInt(data, eid);
    offset += writeBigEndianShort(data + offset, x);
    offset += writeBigEndianShort(data + offset, y);
    offset += writeBigEndianShort(data + offset, z);
    data[offset++] = toAngle(yaw);
    data[offset++] = toAngle(pitch);
    data[offset++] = onGround;
    this->send(data, offset, UPDATE_ENTITY_POSITION_AND_ROTATION);
}

UUID_t player::getUUID() const{
    return entity::getUUID();
}

void player::spawnEntity(const entity* ent){
    byte data[(MAX_VAR_INT * 3) + sizeof(UUID_t) + (sizeof(double) * 3) + (sizeof(angle_t) * 3) + (sizeof(short) * 3)];
    size_t offset = writeVarInt(data, ent->getEid());
    {
        UUID_t uid = ent->getUUID();
        memcpy(data + offset, &uid, sizeof(UUID_t));
        offset += sizeof(UUID_t);
    }
    offset += writeVarInt(data + offset, ent->getEntityType());
    offset += writeBigEndianDouble(data + offset, ent->getX());
    offset += writeBigEndianDouble(data + offset, ent->getY());
    offset += writeBigEndianDouble(data + offset, ent->getZ());
    data[offset++] = toAngle(ent->getPitch());
    data[offset++] = toAngle(ent->getYaw());
    data[offset++] = toAngle(ent->getHeadYaw());
    offset += writeVarInt(data + offset, ent->getEntityData());
    offset += sizeof(short) * 3;
    this->send(data, offset, SPAWN_ENTITY);
}

void player::spawnPlayer(const player* p){
    spdlog::debug("Spawning player {}({}) for player{}({})", p->getUUID(), p->getIndex(), this->getUUID(), this->getIndex());
    {
        byte flag = 0x01 | 0x04 | 0x08 | 0x10;
        byte* data = new byte[1 + MAX_VAR_INT + (sizeof(UUID_t) + (4 * MAX_VAR_INT) + 17)];
        size_t offset = 0;
        data[offset++] = flag;
        offset += writeVarInt(data + offset, 1);
        *(data + offset) = p->getUUID();
        offset += sizeof(UUID_t);
        offset += writeString(data + offset, p->getUsername(), strlen(p->getUsername()));
        offset += writeVarInt(data + offset, 0);
        offset += writeVarInt(data + offset, 0);
        data[offset++] = true;
        offset += writeVarInt(data + offset, p->getPing());
        this->send(data, offset, PLAYER_INFO_UPDATE);
        delete[] data;
    }
    {
        byte data[MAX_VAR_INT + sizeof(UUID_t) + (sizeof(double) * 3) + (sizeof(angle_t) * 2)];
        size_t offset = writeVarInt(data, p->getEid());
        {
            UUID_t uid = p->getUUID();
            memcpy(data + offset, &uid, sizeof(UUID_t));
            offset += sizeof(UUID_t);
        }
        offset += writeBigEndianDouble(data + offset, p->getX());
        offset += writeBigEndianDouble(data + offset, p->getY());
        offset += writeBigEndianDouble(data + offset, p->getZ());
        data[offset++] = toAngle(p->getPitch());
        data[offset++] = toAngle(p->getYaw());
        this->send(data, offset, SPAWN_PLAYER);
    }
}

void player::removeEntity(const entity* ent){
    byte data[MAX_VAR_INT * 2];
    size_t offset = writeVarInt(data, 1);
    offset += writeVarInt(data + offset, ent->getEid());
    this->send(data, offset, REMOVE_ENTITIES);
}

void player::removePlayer(const player* p){
    //FIXME doesn't update player list(possible UUID collision)
    spdlog::debug("Removing player {}({}) for player{}({})", p->getUUID(), p->getIndex(), this->getUUID(), this->getIndex());
    {
        byte data[MAX_VAR_INT + sizeof(UUID_t)];
        size_t offset = writeVarInt(data, 1);
        *(UUID_t*)(data + offset) = p->getUUID();
        offset += sizeof(UUID_t);
        this->send(data, offset, PLAYER_INFO_REMOVE);
    }
    {
        byte data[2 + MAX_VAR_INT + sizeof(UUID_t)];
        size_t offset = 0;
        data[offset++] = 0x08;
        offset += writeVarInt(data + offset, 1);
        *(UUID_t*)(data + offset) = p->getUUID();
        offset += sizeof(UUID_t);
        data[offset++] = false;
        this->send(data, offset, PLAYER_INFO_UPDATE);
    }
    this->removeEntity(p);
}

bool player::isOnGround() const{
    return this->onGround;
}
