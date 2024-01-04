#include "mcr.hpp"

extern "C" {
    #include <unistd.h>
    #include <stdlib.h>
    #include "../C/regionParser.h"
    #include "../../cNBT/nbt.h"
}

minecraftRegion::minecraftRegion(const char* path){
    this->height = 256;
    this->width = 512;
    this->length = 512;
    this->spawnCount = 1;
    this->spawns = (position*)malloc(sizeof(position));
    this->spawns[0] = toPosition(160, 160, 160);
    FILE* f = fopen(path, "rb");
    if(f == NULL){
        throw "Could not open file";
    }
    chunk* rawChunks = getChunks(f);
    fclose(f);
    for(int chunkX = 0; chunkX < 32; chunkX++){
        for(int chunkZ = 0; chunkZ < 32; chunkZ++){
            int i = coordsToIndex(chunkX, chunkZ);
            if(!chunkIsNull(rawChunks[i])){
                size_t sectionCount = 0;
                palettedContainer* sections = getSections(rawChunks + i, &sectionCount);
                
            }
        }
    }
}

minecraftRegion::~minecraftRegion(){
    //TODO
}
