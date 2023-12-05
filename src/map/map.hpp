#ifndef MAP_HPP
#define MAP_HPP

class map{
    public:
        virtual unsigned int getWidth(); 
        virtual unsigned int getHeight(); 
        virtual unsigned int getLength();
        virtual unsigned int* getPalette();
        virtual unsigned int getBlock(unsigned int x, unsigned int y, unsigned int z);
    protected:
        unsigned int width;
        unsigned int height;
        unsigned int length;
        unsigned int* palette;
        unsigned int*** blocks; //nullable 3d array of palette pointers
};

class minecraftMap : public map{
    public:
        minecraftMap(map* source);
};

#endif