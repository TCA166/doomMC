#ifndef MINECRAFTMAP_HPP
#define MINECRAFTMAP_HPP

#include "map.hpp"

#define statesSize 4096

class minecraftMap : public map{
    public:
        /*!
         @brief copies over all stats from source using the methods from map class
         @param source the map to copy over
        */
        minecraftMap(map* source);
        ~minecraftMap();
        /*!
         @brief gets a section of the map
         @param chunkX the x coordinate of the chunk
         @param chunkZ the z coordinate of the chunk
         @param number the y coordinate of the chunk
         @return a palettedContainer containing the section. The palette is the same for all sections so don't free it
        */
        palettedContainer getSection(unsigned int chunkX, unsigned int chunkZ, unsigned int number);
        position getSpawn();
};

#endif
