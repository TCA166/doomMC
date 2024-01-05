#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <zlib.h>

#include "networkingMc.h"
#include "mcTypes.h"

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
    result.len = size;
    byte* input = calloc(size, sizeof(byte));
    int nRead = 0;
    //While we have't read the entire packet
    while(nRead < size){
        //Read the rest of the packet
        int r = read(socketFd, input + nRead, size - nRead);
        if(r == 0 || (r == -1 && (errno != EAGAIN && errno != EWOULDBLOCK))){
            free(input);
            return result;
        }
        nRead += r;
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
    result.packetId = readVarInt(data, &index);
    result.size -= 1; //packetId will should take up a byte at most
    result.data = calloc(result.size, sizeof(byte));
    memcpy(result.data, data + index, result.size);
    if(alloc){ //we discard the const since actually we no longer have a const value
        free((byte*)data);
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
