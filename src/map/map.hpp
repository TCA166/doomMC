#ifndef MAP_HPP
#define MAP_HPP

#include "../C/mcTypes.h"

class map{
    public:
        unsigned int getWidth(); 
        unsigned int getHeight(); 
        unsigned int getLength();
        int32_t* getPalette();
        unsigned int getBlock(unsigned int x, unsigned int y, unsigned int z);
        size_t getPaletteSize();
    protected:
        unsigned int width; //x
        unsigned int height; //y
        unsigned int length;  //z
        size_t paletteSize;
        int32_t* palette;
        int32_t*** blocks;
};

#endif