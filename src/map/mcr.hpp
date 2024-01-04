#ifndef MCR_HPP

#define MCR_HPP

#include "map.hpp"

class minecraftRegion : public map{
    public:
        minecraftRegion(const char* path);
        virtual ~minecraftRegion();
    private:

};

#endif
