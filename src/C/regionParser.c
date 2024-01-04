#include "regionParser.h"
#include <stdlib.h>

int handleFirstSegment(chunk* output, FILE* regionFile, char* regionFileName){
    //so the numbers are stored as big endian AND as int24
    byte bytes[3];
    size_t s1 = fread(&bytes, 1, 3, regionFile);
    output->offset = (bytes[0] << 16) + (bytes[1] << 8) + bytes[2];
    //do an offset check
    long pos = ftell(regionFile);
    fseek(regionFile, 0, SEEK_END);
    if(output->offset > ftell(regionFile)){
        output->offset = -1;
        return 1;
    }
    fseek(regionFile, pos, SEEK_SET);
    //Luckily reading a single byte is simpler
    size_t s2 = fread(&output->sectorCount, 1, 1, regionFile);
    //0 if everything is fine
    return (s1 + s2 == 4) - 1 || (output->offset == 0 && output->sectorCount == 0);
}

int handleSecondSegment(chunk* output, FILE* regionFile){
    byte bytes[4];
    size_t s1 = fread(&bytes, 1, 4, regionFile);
    output->timestamp = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
    return (s1 == 4) - 1 && output->timestamp > 0;
}

chunk* getChunks(FILE* regionFile){
    chunk* chunks = (chunk*)malloc(sizeof(chunk) * chunkN);
    fseek(regionFile, 0, SEEK_SET);
    //despite what it might seem with these loops we are doing a simple linear go through the file
    //first there is a 4096 byte long section of 1024 4 bit fields. Each field is made up of 3 big endian encoded int24s and a single int8
    for(int i = 0; i < chunkN; i++){
        chunk newChunk;
        if(handleFirstSegment(&newChunk, regionFile, "region file") != 0){
            if(chunkIsNull(newChunk)){
                newChunk.byteLength = 0;
                newChunk.data = NULL;
                newChunk.sectorCount = 0;
                newChunk.timestamp = 0;
            }
            else{
                return NULL;
            }
        }
        chunks[i] = newChunk;
    }
    //Then there's an equally long section made up of 1024 int32 timestamps
    for(int i = 0; i < chunkN; i++){
        if(handleSecondSegment(&chunks[i], regionFile) != 0 && !chunkIsNull(chunks[i])){
            return NULL;
        }
    }
    //Then there's encoded chunk data in n*4096 byte long chunks
    //Each of these chunks is made up of a single int32 field that contains the length of the preceding compressed data
    //foreach chunk we have extracted so far
    for(int i = 0; i < chunkN; i++){
        if(!chunkIsNull(chunks[i])){ //if the chunk isn't NULL
            //find the corresponding section
            if(fseek(regionFile, segmentLength * chunks[i].offset, SEEK_SET) != 0){
                return NULL;
            } 
            //get the byteLength
            byte bytes[4];
            if(fread(&bytes, 1, 4, regionFile) != 4){
                return NULL;
            }
            chunks[i].byteLength = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
            //get the compression type
            if(fread(&chunks[i].compression, 1, 1, regionFile) != 1){
                return NULL;
            }
            //fseek(regionFile, 2, SEEK_CUR);
            //Then get the data
            chunks[i].byteLength += 5;
            byte* data = (byte*)malloc(chunks[i].byteLength);
            if(data == NULL){
                return NULL;
            }
            if(fread(data, 1, chunks[i].byteLength, regionFile) != chunks[i].byteLength){
                return NULL;
            }
            chunks[i].data = data;
        }
    }
    return chunks;
}