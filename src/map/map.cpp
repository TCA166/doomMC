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

unsigned int* map::getPalette(){
    return this->palette;
}


