#include "mcr.hpp"
#include <spdlog/spdlog.h>

extern "C" {
    #include <unistd.h>
    #include <stdlib.h>
    #include "../C/regionParser.h"
    #include "../../cNBT/nbt.h"
}

minecraftRegion::minecraftRegion(const char* path, cJSON* version){
    this->height = 256;
    this->width = 512;
    this->length = 512;
    this->blocks = (int32_t***)malloc(sizeof(int32_t**) * this->width);
    for(unsigned int x = 0; x < this->width; x++){
        this->blocks[x] = (int32_t**)malloc(sizeof(int32_t*) * this->height);
        for(unsigned int y = 0; y < this->height; y++){
            this->blocks[x][y] = (int32_t*)calloc(this->length, sizeof(int32_t));
        }
    }
    this->spawnCount = 1;
    this->spawns = (position*)malloc(sizeof(position));
    this->spawns[0] = toPosition(160, 160, 160);
    this->palette = NULL;
    this->paletteSize = 0;
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
                palettedContainer* sections = getSections(rawChunks + i, &sectionCount, version);
                free(rawChunks[i].data);
                if(sections == NULL){
                    throw "Could not get sections";
                }
                for(size_t section = 0; section < sectionCount; section++){
                    //merge the palette
                    this->palette = (int32_t*)realloc(this->palette, sizeof(int32_t) * (this->paletteSize + sections[section].paletteSize));
                    for(size_t j = 0; j < sections[section].paletteSize; j++){
                        bool skip = false;
                        for(size_t n = 0; n < this->paletteSize; n++){
                            if(this->palette[n] == sections[section].palette[j]){
                                sections[section].palette[j] = n;
                                skip = true;
                                break;
                            }
                        }
                        if(!skip){
                            this->palette[this->paletteSize] = sections[section].palette[j];
                            sections[section].palette[j] = this->paletteSize;
                            this->paletteSize++;
                        }
                    }
                    if(sections[section].states != NULL){
                        for(int y = 0; y < 16; y++){
                            for(int x = 0; x < 16; x++){
                                for(int z = 0; z < 16; z++){
                                    int32_t block = sections[section].states[statesFormula(x, y, z)];
                                    if(block != 0){
                                        this->blocks[x + (chunkX * 16)][y + (section * 16)][z + (chunkZ * 16)] = sections[section].palette[block];
                                    }
                                }
                            }
                        }
                    }
                    free(sections[section].states);
                    free(sections[section].palette);
                }
                free(sections);
            }
        }
    }
    free(rawChunks);
    spdlog::info("Loaded region file");
}

minecraftRegion::~minecraftRegion(){
    //TODO
}
