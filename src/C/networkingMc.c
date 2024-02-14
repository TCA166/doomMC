#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <zlib.h>

#include <curl/curl.h>
#include "../../cJSON/cJSON.h"

#include "networkingMc.h"
#include "mcTypes.h"

#define API_PROFILE_URL "https://api.mojang.com/users/profiles/minecraft/"
#define API_SKIN_URL "https://sessionserver.mojang.com/session/minecraft/profile/"
#define COMBINED_API_URL "https://api.ashcon.app/mojang/v2/user/"

static byteArray readSocket(int socketFd){
    byteArray result = nullByteArray;
    int size = 0;
    int position = 0;
    while(true){
        byte curByte = 0;
        if(read(socketFd, &curByte, 1) < 1){
            return result;
        }
        size |= (curByte & SEGMENT_BITS) << position;
        if((curByte & CONTINUE_BIT) == 0) break;
        position += 7;
    }
    if(size == 0 || size > MAX_PACKET_SIZE){
        errno = ERANGE;
        return result;
    }
    result.len = size;
    byte* input = calloc(size, sizeof(byte));
    int nRead = 0;
    //While we have't read the entire packet
    while(nRead < size){
        //FIXME possible infinite loop in case size isn't actually sent
        //Read the rest of the packet
        int r = read(socketFd, input + nRead, size - nRead);
        if(r < 1){
            if(r == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)){
                free(input);
                return result;
            }
        }
        else{
            nRead += r;
        }        
    }
    result.bytes = input;
    return result;
}

static packet parsePacket(const byteArray* dataArray, int compression){
    packet result = nullPacket;
    int index = 0;
    bool alloc = false;
    byte* data = dataArray->bytes;
    if(compression > NO_COMPRESSION){
        uLongf dataLength = (uLongf)readVarInt(data, &index);
        int compressedLen = dataArray->len - index;
        if(dataLength != 0){
            byte* uncompressed = calloc(dataLength, sizeof(byte));
            alloc = true;
            if(uncompress(uncompressed, &dataLength, data + index, compressedLen) != Z_OK){
                free(uncompressed);
                return nullPacket;
            } 
            data = uncompressed;
            index = 0;
            result.size = dataLength;
        }
        else{
            result.size = compressedLen;
        }
    }
    else{
        result.size = dataArray->len;
    }
    int32_t packetId = readVarInt(data, &index);
    if(packetId > MAX_PACKET_ID){
        if(alloc){
            free(data);
        }
        errno = ERANGE;
        return nullPacket;
    }
    result.packetId = packetId;
    result.size -= 1; //packetId will should take up a byte at most
    result.data = calloc(result.size, sizeof(byte));
    memcpy(result.data, data + index, result.size);
    if(alloc){ //we discard the const since actually we no longer have a const value
        free(data);
    }
    return result;
}

packet readPacket(int socketFd, int compression){
    byteArray data = readSocket(socketFd);
    if(data.bytes == NULL){
        return nullPacket;
    }
    packet result = parsePacket(&data, compression);
    free(data.bytes);
    return result;
}

ssize_t sendPacket(int socketFd, int size, int packetId, const byte* data, int compression){
    if(compression <= NO_COMPRESSION){ //packet type without compression
        byte p[MAX_VAR_INT];
        size_t pSize = writeVarInt(p, packetId);
        byte s[MAX_VAR_INT];
        size_t sSize = writeVarInt(s, size + pSize);
        if(write(socketFd, s, sSize) == -1 || write(socketFd, p, pSize) == -1){
            return -1;
        }
        if(data != NULL){
            return write(socketFd, data, size);
        }
        return sSize + 1;
    }
    else{
        byte* dataToCompress = calloc(MAX_VAR_INT + size, sizeof(byte));
        size_t offset = writeVarInt(dataToCompress, packetId);
        memcpy(dataToCompress + offset, data, size);
        //ok so now we can compress
        int dataLength = offset + size;
        uLongf destLen = dataLength;
        if(dataLength > compression){
            destLen = compressBound(dataLength);
            byte* compressed = calloc(destLen, sizeof(byte));
            int res = compress(compressed, &destLen, dataToCompress, dataLength);
            free(dataToCompress);
            if(res != Z_OK){
                free(compressed);
                errno = res;
                return -2;
            }
            dataToCompress = compressed;
        }
        else{
            dataLength = 0;
        }
        byte* packet = calloc(destLen + (MAX_VAR_INT * 2), sizeof(byte));
        byte l[MAX_VAR_INT];
        size_t sizeL = writeVarInt(l, dataLength);
        size_t sizeP = writeVarInt(packet, destLen + sizeL);
        memcpy(packet + sizeP, l, sizeL);
        memcpy(packet + sizeP + sizeL, dataToCompress, destLen);
        free(dataToCompress);
        ssize_t res = write(socketFd, packet, destLen + sizeP + sizeL);
        free(packet);
        return res;
    }
}

static size_t writeToString(void *ptr, size_t size, size_t nmemb, struct string *s){
    size_t newLen = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, newLen + 1);
    if(s->ptr == NULL){
        return 0;
    }
    memcpy(s->ptr + s->len, ptr, size*nmemb);
    s->ptr[newLen] = '\0';
    s->len = newLen;
    return size*nmemb;
}

skin_t getPlayerSkin(const char* username){
    struct string res = {NULL, 0};
    CURL* curl = curl_easy_init();
    if(curl == NULL){
        return emptySkin;
    }
    {
        char* url = calloc(sizeof(COMBINED_API_URL) + strlen(username) + 1, sizeof(char));
        strcpy(url, COMBINED_API_URL);
        strcat(url, username);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);
        if(curl_easy_perform(curl) != CURLE_OK){
            free(url);
            curl_easy_cleanup(curl);
            return emptySkin;
        }
        free(url);
    }
    curl_easy_cleanup(curl);
    cJSON* json = cJSON_Parse(res.ptr);
    free(res.ptr);
    res.ptr = NULL;
    if(json == NULL){
        return emptySkin;
    }
    cJSON* textures = cJSON_GetObjectItem(json, "textures");
    if(textures == NULL){
        cJSON_Delete(json);
        return emptySkin;
    }
    cJSON* raw = cJSON_GetObjectItem(textures, "raw");
    if(raw == NULL){
        cJSON_Delete(json);
        return emptySkin;
    }
    cJSON* value = cJSON_GetObjectItem(raw, "value");
    if(value == NULL){
        cJSON_Delete(json);
        return emptySkin;
    }
    char* skin = calloc(strlen(value->valuestring) + 1, sizeof(char));
    strcpy(skin, value->valuestring);
    skin_t result = {NULL, skin};
    cJSON* signature = cJSON_GetObjectItem(raw, "signature");
    if(signature != NULL){
        result.signature = calloc(strlen(signature->valuestring) + 1, sizeof(char));
        strcpy(result.signature, signature->valuestring);
    }
    cJSON_Delete(json);
    return result;
}
