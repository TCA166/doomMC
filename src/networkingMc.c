#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
        if(r < 1){
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
