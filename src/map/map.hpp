#ifndef MAP_HPP
#define MAP_HPP

extern "C"{
    #include "../C/mcTypes.h"
}

#define statesSize 4096

class map{
    public:
        unsigned int getWidth() const; 
        unsigned int getHeight() const; 
        unsigned int getLength() const;
        /*!
         @brief Gets the width or length of the map, whichever is greater
         @return the width or length of the map, whichever is greater
        */
        unsigned int getBound() const;
        const int32_t* getPalette() const;
        unsigned int getBlock(unsigned int x, unsigned int y, unsigned int z) const;
        size_t getPaletteSize() const;
        /*!
         @brief gets a section of the map
         @param chunkX the x coordinate of the chunk
         @param chunkZ the z coordinate of the chunk
         @param number the y coordinate of the chunk
         @return a palettedContainer containing the section. The palette is the same for all sections so don't free it
        */
        palettedContainer getSection(unsigned int chunkX, unsigned int chunkZ, unsigned int number) const;
        position getSpawn() const;
        virtual ~map();
    protected:
        unsigned int width; //x
        unsigned int height; //y
        unsigned int length;  //z
        size_t paletteSize;
        int32_t* palette;
        int32_t*** blocks;
        position* spawns;
        size_t spawnCount;
};

#endif