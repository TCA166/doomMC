#include "client.hpp"
#include "server.hpp"

extern "C" {
    #include <unistd.h>
    #include <stdlib.h>
    #include <string.h>
    #include "../cJSON/cJSON.h"
    #include <errno.h>
}

client::client(server* server, int fd, state_t state, char* username, int compression, int32_t protocol, int index) : client(server, fd, state, username, compression, protocol){
    this->index = index;
}

client::client(server* server, int fd, state_t state, char* username, int compression, int32_t protocol){
    this->serv = server;
    this->fd = fd;
    this->state = state;
    this->username = username;
    this->compression = compression;
    this->protocol = protocol;
}

client::client(server* server, int fd, int index){
    this->serv = server;
    this->fd = fd;
    this->state = NONE_STATE;
    this->username = NULL;
    this->compression = NO_COMPRESSION;
    this->protocol = 0;
    this->index = index;
}

client::client(){
    this->fd = 0;
    this->state = NONE_STATE;
    this->username = NULL;
    this->compression = NO_COMPRESSION;
    this->protocol = 0;
}

client::~client(){
    this->disconnect();
    free(this->username);
}

void client::disconnect(){
    if(this->fd == -1){
        return;
    }
    if(this->state == PLAY_STATE){
        this->send(NULL, 0, DISCONNECT_PLAY); //send disconnect packet
    }
    else if(this->state == LOGIN_STATE){
        this->send(NULL, 0, DISCONNECT_LOGIN); //send disconnect packet
    }
    close(this->fd);
    this->fd = -1;
    this->serv->disconnectClient(this->index);
}

packet client::getPacket(){
    packet p = readPacket(this->fd, this->compression);
    return p;
}

int client::send(byte* data, int length, byte packetId){
    int res = sendPacket(this->fd, length, packetId, data, this->compression);
    if(res == -1 && errno == EPIPE){
        this->disconnect();
    }
    return res;
}

int client::handlePacket(packet* p){
    int offset = 0;
    switch(this->state){
        case NONE_STATE:{
            if(p->packetId != HANDSHAKE){
                return 0;
            }
            this->protocol = readVarInt(p->data, &offset);
            int32_t serverAddressLength = readVarInt(p->data, &offset);
            offset += serverAddressLength + sizeof(unsigned short);
            int32_t nextState = readVarInt(p->data, &offset);
            this->state = (state_t)nextState;
            break;
        }
        case STATUS_STATE:{
            if(p->packetId == STATUS_REQUEST){
                cJSON* status = this->serv->getMessage();
                cJSON* version = cJSON_GetObjectItemCaseSensitive(status, "version");
                cJSON* protocol = cJSON_GetObjectItemCaseSensitive(version, "protocol");
                cJSON_SetIntValue(protocol, this->protocol);
                cJSON* players = cJSON_GetObjectItemCaseSensitive(status, "players");
                cJSON_GetObjectItemCaseSensitive(players, "online")->valueint = this->serv->getPlayerCount();
                char* json = cJSON_Print(status);
                size_t sizeJson = strlen(json);
                byte data[sizeJson + 1 + MAX_VAR_INT];
                size_t dataSz = writeString(data, json, sizeJson);
                free(json);
                return this->send(data, dataSz, STATUS_RESPONSE);
            }
            else if(p->packetId == PING_REQUEST){
                int64_t val = readLong(p->data, &offset);
                return this->send((byte*)&val, sizeof(val), PING_RESPONSE);
            }
            else{
                return 0;
            }
        }
        case LOGIN_STATE:{
            if(p->packetId == LOGIN_START){
                this->username = readString(p->data, &offset);
                size_t len = strlen(this->username);
                this->uuid = readUUID(p->data, &offset);
                byte data[sizeof(uuid) + len + 1 + (MAX_VAR_INT * 2)];
                *(UUID_t*)&data = uuid;
                size_t sz1 = writeString(data + sizeof(uuid), this->username, len);
                size_t sz2 = writeVarInt(data + sizeof(uuid) + sz1, 0);
                this->send(data, sizeof(uuid) + sz1 + sz2, LOGIN_SUCCESS);
                if(this->protocol <= NO_CONFIG){
                    this->state = PLAY_STATE;
                    this->serv->addToLobby(this);
                }
            }
            else if(p->packetId == LOGIN_ACKNOWLEDGED){
                this->state = CONFIG_STATE;
                this->sendRegistryCodec();
                this->sendTags();
                this->sendFeatureFlags();
                this->send(NULL, 0, FINISH_CONFIGURATION);
            }
            else{
                return 0;
            }
            break;
        }
        case CONFIG_STATE:{
            if(p->packetId == FINISH_CONFIGURATION_2){
                this->state = PLAY_STATE;
                this->serv->addToLobby(this);
            }
            break;
        }
        default:{
            return 0;
        }
    }
    return 1;
}

int client::getCompression() const{
    return this->compression;
}

int client::getFd(){
    return this->fd;
}

state_t client::getState() const{
    return this->state;
}

void client::setUsername(char* username) {
    this->username = username;
}

player* client::toPlayer(){
    player* p = new player(this->serv, this->fd, this->state, this->username, this->compression, this->protocol);
    return p;
}

void client::sendRegistryCodec(){
    if(this->state != CONFIG_STATE){
        throw "Invalid state";
    }
    const byteArray* codec = this->serv->getRegistryCodec();
    byte data[codec->len];
    memcpy(data, codec->bytes, codec->len);
    this->send(data, codec->len, REGISTRY_DATA);
}

void client::sendFeatureFlags(){
    const char* minecraftVanillaFlag = "minecraft:vanilla";
    byte data[(MAX_VAR_INT * 2) + sizeof(minecraftVanillaFlag)];
    size_t offset = writeVarInt(data, 1);
    offset += writeString(data + offset, minecraftVanillaFlag, sizeof(minecraftVanillaFlag));
    if(protocol > NO_CONFIG){
        this->send(data, offset, FEATURE_FLAGS);
    }
    else{
        this->send(data, offset, FEATURE_FLAGS_PLAY);
    }
}

void client::sendTags(){
    const char* blockTag = "minecraft:block";
    const char* indestructibleTag = "features_cannot_replace";
    byte data[(MAX_VAR_INT * 3) + sizeof(indestructibleTag) + sizeof(blockTag)];
    size_t offset = writeVarInt(data, 2);
    offset += writeString(data + offset, blockTag, sizeof(blockTag));
    offset += writeVarInt(data + offset, 0);
    offset += writeString(data + offset, indestructibleTag, sizeof(indestructibleTag));
    //TODO figure out a specific list of allowed block ids on the server
    offset += writeVarInt(data + offset, 0);
    if(this->state == CONFIG_STATE){
        this->send(data, offset, UPDATE_TAGS);
    }
    else if(this->state == PLAY_STATE){
        this->send(data, offset, UPDATE_TAGS_PLAY);
    }
}

const char* client::getUsername() const{
    return this->username;
}

int client::getIndex(){
    return this->index;
}

void client::setIndex(int index){
    this->index = index;
}
