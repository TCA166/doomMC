#include "map.hpp"
#include <stdlib.h>

unsigned int map::getBlock(unsigned int x, unsigned int y, unsigned int z){
    if(x >= this->width || y >= this->height || z >= this->length){
        throw "Out of bounds";
    }
    return this->blocks[x][y][z];
}

unsigned int map::getWidth(){
    return this->width;
}

unsigned int map::getHeight(){
    return this->height;
}

unsigned int map::getLength(){
    return this->length;
}

int32_t* map::getPalette(){
    return this->palette;
}

size_t map::getPaletteSize(){
    return this->paletteSize;
}

size_t map::getSpawnCount(){
    return this->spawnCount;
}
