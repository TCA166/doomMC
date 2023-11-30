#include "client.hpp"
#include <unistd.h>

extern "C" {
    #include <stdlib.h>
    #include <string.h>
    #include "../cJSON/cJSON.h"
    #include <errno.h>
}

client::client(int fd, state_t state, char* username, int compression, int32_t protocol){
    this->fd = fd;
    this->state = state;
    this->username = username;
    this->compression = compression;
    this->protocol = protocol;
}

client::client(int fd){
    this->fd = fd;
    this->state = NONE_STATE;
    this->username = NULL;
    this->compression = NO_COMPRESSION;
    this->protocol = 0;
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
    close(this->fd);
    this->fd = -1;
}

packet client::getPacket(){
    packet p = readPacket(this->fd, this->compression);
    return p;
}

int client::send(byte* data, int length, byte packetId){
    return sendPacket(this->fd, length, packetId, data, this->compression);
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
                //TODO finish the status request
                byte data[256];
                char json[] = "{\"version\":{\"name\":\"N/A\",\"protocol\":754},\"players\":{\"max\":100,\"online\":0,\"sample\":[]},\"description\":{\"text\":\"Hello world\"}}";
                return this->send(data, writeString(data, json, sizeof(json)), STATUS_RESPONSE);
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
                //TODO fix
                this->username = readString(p->data, &offset);
                size_t len = strlen(this->username);
                UUID_t uuid = readUUID(p->data, &offset);
                byte data[sizeof(uuid) + len + 1 + MAX_VAR_INT];
                *(UUID_t*)&data = uuid;
                memcpy(data + sizeof(uuid), this->username, len + 1);
                size_t size = writeVarInt(data + sizeof(uuid) + len + 1, 0);
                this->send(data, sizeof(uuid) + len + 1 + size, LOGIN_SUCCESS);
            }
            break;
        }
    }
    return 1;
}

int client::getCompression(){
    return this->compression;
}

int client::getFd(){
    return this->fd;
}

byte client::getState(){
    return this->state;
}

void client::setUsername(char* username) {
    this->username = username;
}
