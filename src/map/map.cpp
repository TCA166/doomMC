#include "map.hpp"
#include <stdlib.h>
#include <stdexcept>

extern "C" {
    #include <time.h>
}

unsigned int map::getBlock(unsigned int x, unsigned int y, unsigned int z) const{
    if(x >= this->width || y >= this->height || z >= this->length){
        throw std::invalid_argument("Block coordinates out of bounds");
    }
    return this->blocks[x][y][z];
}

unsigned int map::getWidth() const{
    return this->width;
}

unsigned int map::getHeight() const{
    return this->height;
}

unsigned int map::getLength() const{
    return this->length;
}

unsigned int map::getBound() const{
    return this->width > this->length ? this->width : this->length;
}

const int32_t* map::getPalette() const{
    return this->palette;
}

size_t map::getPaletteSize() const{
    return this->paletteSize;
}

palettedContainer map::getSection(unsigned int chunkX, unsigned int chunkZ, unsigned int number) const{
    if(chunkX >= (this->width / 16) + 1 || chunkZ >= (this->length / 16) + 1){
        throw std::invalid_argument("Chunk coordinates out of bounds");
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

position map::getSpawn() const{
    srand((unsigned) time(NULL));
    int random = rand();
    return this->spawns[random % this->spawnCount];
}

map::~map(){
    for(unsigned int x = 0; x < this->width; x++){
        for(unsigned int y = 0; y < this->height; y++){
            free(this->blocks[x][y]);
        }
        free(this->blocks[x]);
    }
    free(this->blocks);
    free(this->palette);
    free(this->spawns);
}
