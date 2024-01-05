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
    this->teleportId = 0;
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

void player::setCenterChunk(int32_t x, int32_t z){
    byte data[MAX_VAR_INT * 2];
    size_t offset = writeVarInt(data, x);
    offset += writeVarInt(data + offset, z);
    this->send(data, offset, SET_CENTER_CHUNK);
}

void player::setLocation(double x, double y, double z){
    spdlog::debug("Setting location of player {} to {}, {}, {}", this->username, x, y, z);
    this->setCenterChunk((int32_t)floor(x / 16), (int32_t)floor(z / 16));
    {
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
        offset += writeVarInt(data + offset, this->teleportId++);
        this->send(data, offset, SYNCHRONIZE_PLAYER_POSITION);
    }
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
    byte* packet = new byte[msgLen + 3];
    size_t offset = 0;
    packet[offset] = TAG_STRING;
    offset += writeBigEndianUShort(packet, msgLen);
    memcpy(packet + offset, message, msgLen);
    offset += msgLen;
    packet[offset] = 0;
    offset++;
    this->send(packet, offset, SYSTEM_CHAT_MESSAGE);
    delete packet;
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
        if(this->protocol <= NO_CONFIG){
            {
                const char* dimensionName = (const char*)"minecraft:overworld";
                const byteArray* registryCodec = this->currentLobby->getRegistryCodec();
                byte* data = new byte[(sizeof(int32_t) * 2) + 10 + (20 * 3) + (MAX_VAR_INT * 5) + registryCodec->len];
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
                memcpy(data + offset, registryCodec->bytes, registryCodec->len);
                offset += registryCodec->len;
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
                this->send(data, offset - 1, LOGIN_PLAY); //TODO figure out why the -1 is needed
                delete data;
            }
        }
        else{//TODO implement new LOGIN_PLAY format

        }
    }
    //https://wiki.vg/Protocol_FAQ#What.27s_the_normal_login_sequence_for_a_client.3F
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
        data[offset] = 24;
        offset++;
        this->send(data, offset, ENTITY_EVENT);
    }
    //send commands
    {
        byte data[MAX_VAR_INT * 2];
        size_t offset = writeVarInt(data, 0);
        offset += writeVarInt(data + offset, 0);
        this->send(data, offset, COMMANDS);
    }
    {//update player list
        
        byte flag = 0x01 | 0x04 | 0x08 | 0x10;
        unsigned int playerNum = this->currentLobby->getPlayerCount();
        byte* data = new byte[1 + MAX_VAR_INT + (sizeof(UUID_t) + (4 * MAX_VAR_INT) + 17) * playerNum];
        size_t offset = 0;
        data[offset] = flag;
        offset++;
        offset += writeVarInt(data + offset, playerNum);
        for(unsigned int i = 0; i < playerNum; i++){
            const player* p = this->currentLobby->getPlayer(i);
            *(data + offset) = p->getUUID();
            offset += sizeof(UUID_t);
            offset += writeString(data + offset, p->getUsername(), strlen(p->getUsername()));
            offset += writeVarInt(data + offset, 0);
            offset += writeVarInt(data + offset, 0);
            data[offset] = true;
            offset++;
            offset += writeVarInt(data + offset, 0);
        }
        this->send(data, offset, PLAYER_INFO_UPDATE);
        delete data;
    }
    const map* m = this->currentLobby->getMap();
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
                    free(sections[i].states);
                }
            }
        }
        this->send(NULL, 0, BUNDLE_DELIMITER);
    }
    {//send initialize world border
        byte data[(sizeof(double) * 4) + (MAX_VAR_INT * 4)];
        size_t offset = 0;
        offset += writeBigEndianDouble(data, 0);
        offset += writeBigEndianDouble(data + offset, 0);
        offset += writeBigEndianDouble(data + offset, 0);
        offset += writeBigEndianDouble(data + offset, 1000);
        offset += writeVarInt(data + offset, 0); //TODO should be varLong
        offset += writeVarInt(data + offset, 0);
        offset += writeVarInt(data + offset, 0);
        offset += writeVarInt(data + offset, 0);
        this->send(data, offset, INITIALIZE_WORLD_BORDER);
    }
    position spawn = m->getSpawn();
    this->setCenterChunk((int32_t)floor(positionX(spawn) / 16), (int32_t)floor(positionZ(spawn) / 16));
    //send set default spawn position
    {
        byte data[sizeof(position) + sizeof(float)];
        size_t offset = 0;
        offset += writeBigEndianLong(data + offset, spawn);
        //angle
        offset += writeBigEndianFloat(data + offset, 0);
        this->send(data, offset, SET_DEFAULT_SPAWN_POSITION);
    }
    //then send Set Container Content
    this->setWeapons(this->currentLobby->getWeapons(), this->currentLobby->getAmmo());
    this->setHealth(20);
    this->setLocation((double)positionX(spawn), (double)positionY(spawn), (double)positionZ(spawn));
    spdlog::info("Player {} joined lobby", this->username);
}

int player::handlePacket(packet* p){
    spdlog::debug("Handling packet {} from client {}", p->packetId, this->uuid);
    //TODO sync and expand
    if(this->state != PLAY_STATE){
        return -1;
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
            byte jump = readByte(p->data, &offset);
            jump = jump & 0x1;
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
    const size_t neededLater = ((MAX_VAR_INT + sizeof(int64_t)) * 4) + (MAX_VAR_INT * 3);
    byte* data = (byte*)malloc((sizeof(int32_t) * 2) + (sizeof(int16_t) * 2) + (17));
    size_t offset = 0;
    offset += writeBigEndianInt(data + offset, chunkX);
    offset += writeBigEndianInt(data + offset, chunkZ);
    { //heightmap NBT
        data[offset] = TAG_COMPOUND;
        offset++;
        offset += writeBigEndianShort(data + offset, 0);
        data[offset] = TAG_LONG_ARRAY;
        offset++;
        offset += writeBigEndianShort(data + offset, 15);
        memcpy(data + offset, "MOTION_BLOCKING", 15);
        offset += 15;
        const uint8_t bpe = ceil(log2((sectionCount * 16) + 1));
        const int perLong = (int)(64 / bpe);
        const size_t longCount = (size_t)ceilf((float)256 / (float)perLong);
        //MAYBE actually implment heightmap, for now it just sends a whole bunch of zeroes
        uint64_t* heightmapData = (uint64_t*)calloc(longCount, sizeof(uint64_t));
        size_t bytesSz = longCount * sizeof(int64_t);
        data = (byte*)realloc(data, offset + bytesSz + sizeof(int32_t) + 2 + MAX_VAR_INT);
        offset += writeBigEndianInt(data + offset, longCount);
        for(unsigned int i = 0; i < longCount; i++){
            offset += writeBigEndianLong(data + offset, heightmapData[i]);
        }
        free(heightmapData);
        data[offset] = TAG_INVALID;
        offset++;
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
    data[offset] = true;
    offset++;
    {//bit sets for sky light
        int64_t zero = 0;
        int64_t one = 0xFFFFFFFFFFFFFFFF; //16 times F
        const bitSet empty = {1, &zero};
        const bitSet full = {1, &one};
        offset += writeBitSet(data + offset, &empty);
        offset += writeBitSet(data + offset, &empty);
        offset += writeBitSet(data + offset, &full);
        offset += writeBitSet(data + offset, &full);
    }
    offset += writeVarInt(data + offset, 0); //sky light array length
    offset += writeVarInt(data + offset, 0);
    this->send(data, offset, CHUNK_DATA_AND_UPDATE_LIGHT);
    free(data);
}

void player::disconnect(){
    if(this->fd == -1){
        return;
    }
    spdlog::debug("Disconnecting client {}({})", this->uuid, this->index);
    this->send(NULL, 0, DISCONNECT_PLAY); //send disconnect packet
    this->currentLobby->removePlayer(this);
}

player::~player(){
    spdlog::debug("Deleting player {}({})", this->uuid, this->index);
}

void player::keepAlive(){
    byte data[sizeof(int64_t)];
    size_t offset = writeBigEndianLong(data, (int64_t)time(NULL));
    this->send(data, offset, KEEP_ALIVE);
}

time_t player::getPing(){
    return time(NULL) - this->lastKeepAlive;
}
