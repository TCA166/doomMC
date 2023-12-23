#include "minecraftMap.hpp"
#include <stdlib.h>

minecraftMap::minecraftMap(map* source){
    this->width = source->getWidth();
    this->height = source->getHeight();
    this->length = source->getLength();
    this->paletteSize = source->getPaletteSize();
    this->palette = source->getPalette();
    this->blocks = (int32_t***)calloc(this->width, sizeof(int32_t**));
    for(int x = 0; x < this->width; x++){
        this->blocks[x] = (int32_t**)calloc(this->height, sizeof(int32_t*));
        for(int y = 0; y < this->height; y++){
            this->blocks[x][y] = (int32_t*)calloc(this->length, sizeof(int32_t));
            for(int z = 0; z < this->length; z++){
                this->blocks[x][y][z] = source->getBlock(x, y, z);
            }
        }
    }
}

minecraftMap::~minecraftMap(){
    free(this->blocks);
    free(this->palette);
}

palettedContainer minecraftMap::getSection(unsigned int chunkX, unsigned int chunkZ, unsigned int number){
    if(chunkX >= this->width / 16 || chunkZ >= this->length / 16){
        throw "Out of bounds";
    }
    if(number >= this->height / 16){
        int32_t* single = (int32_t*)calloc(1, sizeof(int32_t));
        return {1, single, NULL};
    }
    palettedContainer container;
    container.palette = this->palette;
    container.paletteSize = this->paletteSize;
    container.states = (int32_t*)calloc(statesSize, sizeof(int32_t));
    //foreach block in the section
    for(int x = 0; x < 16; x++){
        for(int y = 0; y < 16; y++){
            for(int z = 0; z < 16; z++){
                int32_t state =  this->blocks[(chunkX * 16) + x][(number * 16) + y][(chunkZ * 16) + z];
                if(state > 0){
                    container.states[statesFormula(x, y, z)] = state;
                }
            }
        }
    }
    return container;
}