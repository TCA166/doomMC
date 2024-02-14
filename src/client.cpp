#include "client.hpp"
#include "server.hpp"
#include "player.hpp"
#include <spdlog/spdlog.h>

extern "C" {
    #include <unistd.h>
    #include <stdlib.h>
    #include <string.h>
    #include "../cJSON/cJSON.h"
    #include <errno.h>
    #include <time.h>
}

#define clientCompression 20

client::client(server* server, int fd, state_t state, char* username, int compression, int32_t protocol, int index) : client(server, fd, state, username, compression, protocol){
    this->index = index;
}

client::client(server* server, int fd, state_t state, char* username, int compression, int32_t protocol) : client(){
    this->serv = server;
    this->fd = fd;
    this->state = state;
    this->username = username;
    this->compression = compression;
    this->protocol = protocol;
}

client::client(server* server, int fd, int index) : client(){
    this->serv = server;
    this->fd = fd;
    this->index = index;
}

client::client(){
    this->fd = 0;
    this->state = NONE_STATE;
    this->username = NULL;
    this->compression = NO_COMPRESSION;
    this->protocol = 0;
    this->index = -1;
    this->uuid = 0;
}

client::~client(){
    close(this->fd);
    free(this->username);
    spdlog::debug("Client {}({}) destroyed", this->uuid, this->index);
}

void client::disconnect(){
    if(this->index == -1){
        return;
    }
    spdlog::debug("Disconnecting client {}({})", this->uuid, this->index);
    if(this->state == PLAY_STATE){
        spdlog::warn("Unexpected disconnect");
        this->send(NULL, 0, DISCONNECT_PLAY); //send disconnect packet
    }
    else if(this->state == LOGIN_STATE){
        this->send(NULL, 0, DISCONNECT_LOGIN); //send disconnect packet
    }
    this->serv->removeClient(this->index);
    this->index = -1;
}

packet client::getPacket(){
    packet p = readPacket(this->fd, this->compression);
    #ifdef DEBUG_PACKETS
    spdlog::debug("Read packet {} from client {}", p.packetId, this->uuid);
    #endif
    if(packetNull(p)){
        spdlog::warn("Failed to read packet from client {}", this->uuid);
        if(errno == EPIPE){
            this->disconnect();
        }
        else{
            throw std::error_code(errno, std::generic_category());
        }
    }
    return p;
}

void client::send(byte* data, int length, byte packetId){
    #ifdef DEBUG_PACKETS
    spdlog::debug("Sending packet {} to client {}", packetId, this->uuid);
    #endif
    int res = sendPacket(this->fd, length, packetId, data, this->compression);
    if(res < 0){
        spdlog::warn("Sending packet {} to {} failed", packetId, this->uuid);
        if(res == -2){
            spdlog::error("Zlib error: {}", errno);
            errno = ELIBEXEC;
        }
        else if(errno == EPIPE){
            this->disconnect();
        }
        throw std::error_code(errno, std::generic_category());
    }
}

void client::handlePacket(packet* p){
    spdlog::debug("Handling packet {} from client {}", p->packetId, this->uuid);
    int offset = 0;
    switch(this->state){
        case NONE_STATE:{
            if(p->packetId != HANDSHAKE){
                return;
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
                char* json = cJSON_Print(status);
                size_t sizeJson = strlen(json);
                byte* data = new byte[sizeJson + 1 + MAX_VAR_INT];
                size_t dataSz = writeString(data, json, sizeJson);
                free(json);
                this->send(data, dataSz, STATUS_RESPONSE);
                delete[] data;
                return;
            }
            else if(p->packetId == PING_REQUEST){
                int64_t val = readLong(p->data, &offset);
                return this->send((byte*)&val, sizeof(val), PING_RESPONSE);
            }
            else{
                return;
            }
        }
        case LOGIN_STATE:{
            if(p->packetId == LOGIN_START){
                {
                    //send the compression packet, then set the compression level
                    byte data[MAX_VAR_INT];
                    size_t offset = writeVarInt(data, clientCompression);
                    this->send(data, offset, SET_COMPRESSION);
                    this->compression = clientCompression;
                }    
                this->username = readString(p->data, &offset);
                size_t len = strlen(this->username);
                this->uuid = readUUID(p->data, &offset);
                if(this->uuid == 0){
                    this->uuid = createOffline(this->username);
                }
                byte* data = new byte[sizeof(UUID_t) + len + 1 + (MAX_VAR_INT * 2)];
                memcpy(data, &this->uuid, sizeof(UUID_t));
                size_t dataOffset = sizeof(UUID_t);
                dataOffset += writeString(data + dataOffset, this->username, len);
                dataOffset += writeVarInt(data + dataOffset, 0);
                this->send(data, dataOffset, LOGIN_SUCCESS);
                if(this->protocol <= NO_CONFIG){
                    this->state = PLAY_STATE;
                    this->serv->addToLobby(this);
                }
                delete[] data;
            }
            else if(p->packetId == LOGIN_ACKNOWLEDGED){
                this->state = CONFIG_STATE;
                this->sendRegistryCodec();
                this->sendTags();
                this->sendFeatureFlags();
                this->send(NULL, 0, FINISH_CONFIGURATION);
            }
            else{
                return;
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
            return;
        }
    }
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
    int newSocket = dup(this->fd);
    char* username = (char*)malloc(strlen(this->username) + 1);
    strcpy(username, this->username);
    player* p = new player(this->serv, newSocket, this->state, username, this->compression, this->protocol, this->uuid);
    return p;
}

void client::sendRegistryCodec(){
    if(this->state != CONFIG_STATE){
        throw std::invalid_argument("Client is not in config state");
    }
    const byteArray* codec = this->serv->getRegistryCodec();
    byte* data = new byte[codec->len];
    memcpy(data, codec->bytes, codec->len);
    this->send(data, codec->len, REGISTRY_DATA);
    delete[] data;
}

void client::sendFeatureFlags(){
    const char* minecraftVanillaFlag = "minecraft:vanilla";
    byte data[(MAX_VAR_INT * 2) + 17];
    size_t offset = writeVarInt(data, 1);
    offset += writeString(data + offset, minecraftVanillaFlag, 17);
    if(protocol > NO_CONFIG){
        this->send(data, offset, FEATURE_FLAGS);
    }
    else{
        this->send(data, offset, FEATURE_FLAGS_PLAY);
    }
}

void client::sendTags(){
    const byteArray* tags = this->serv->getTags();
    spdlog::debug("Sending tags to client {}", this->uuid);
    if(this->state == CONFIG_STATE){
        this->send(tags->bytes, tags->len, UPDATE_TAGS);
    }
    else if(this->state == PLAY_STATE){
        this->send(tags->bytes, tags->len, UPDATE_TAGS_PLAY);
    }
}

const char* client::getUsername() const{
    return this->username;
}

int client::getIndex() const{
    return this->index;
}

void client::setIndex(int index){
    this->index = index;
}

packet client::getPacket(int timeout){
    packet p = this->getPacket();
    clock_t begin = clock();
    while(packetNull(p)){
        if((double)(clock() - begin) / CLOCKS_PER_SEC >= timeout){
            spdlog::debug("Timed out waiting for packet");
            throw std::runtime_error("Timed out waiting for packet");
        }
        p = this->getPacket();
    }
    spdlog::debug("Got packet {} after waiting", p.packetId);
    return p;
}

UUID_t client::getUUID() const{
    return this->uuid;
}
