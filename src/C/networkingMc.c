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
        if(size + offset > compression){
            byte* compressed = calloc(destLen, sizeof(byte));
            if(compress(compressed, &destLen, dataToCompress, destLen) != Z_OK){
                return -1;
            }
            free(dataToCompress);
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

struct string {
    char *ptr;
    size_t len;
};

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

char* getPlayerSkin(const char* username){
    struct string res = {NULL, 0};
    CURL* curl = curl_easy_init();
    if(curl == NULL){
        return NULL;
    }
    {
        char* url = calloc(sizeof(API_PROFILE_URL) + strlen(username) + 1, sizeof(char));
        strcpy(url, API_PROFILE_URL);
        strcat(url, username);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);
        if(curl_easy_perform(curl) != CURLE_OK){
            free(url);
            curl_easy_cleanup(curl);
            return NULL;
        }
        free(url);
    }
    cJSON* json = cJSON_Parse(res.ptr);
    free(res.ptr);
    res.ptr = NULL;
    if(json == NULL){
        curl_easy_cleanup(curl);
        return NULL;
    }
    cJSON* id = cJSON_GetObjectItem(json, "id");
    if(id == NULL){
        cJSON_Delete(json);
        curl_easy_cleanup(curl);
        return NULL;
    }
    {
        char* skinUrl = calloc(sizeof(API_SKIN_URL) + strlen(id->valuestring) + 1, sizeof(char));
        strcpy(skinUrl, API_SKIN_URL);
        strcat(skinUrl, id->valuestring);
        cJSON_Delete(json);
        curl_easy_setopt(curl, CURLOPT_URL, skinUrl);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);
        res.len = 0;
        if(curl_easy_perform(curl) != CURLE_OK){
            free(skinUrl);
            curl_easy_cleanup(curl);
            return NULL;
        }
        free(skinUrl);
    }
    cJSON* skinJson = cJSON_Parse(res.ptr);
    free(res.ptr);
    cJSON* properties = cJSON_GetObjectItem(skinJson, "properties");
    if(properties == NULL){
        cJSON_Delete(skinJson);
        curl_easy_cleanup(curl);
        return NULL;
    }
    cJSON* skin = cJSON_GetArrayItem(properties, 0);
    if(skin == NULL){
        cJSON_Delete(skinJson);
        curl_easy_cleanup(curl);
        return NULL;
    }
    cJSON* value = cJSON_GetObjectItem(skin, "value");
    if(value == NULL){
        cJSON_Delete(skinJson);
        curl_easy_cleanup(curl);
        return NULL;
    }
    char* skinData = calloc(strlen(value->valuestring) + 1, sizeof(char));
    strcpy(skinData, value->valuestring);
    cJSON_Delete(skinJson);
    curl_easy_cleanup(curl);
    return skinData;
}
