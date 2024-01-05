#ifndef MCR_HPP

#define MCR_HPP

#include "map.hpp"

extern "C" {
    #include "../../cJSON/cJSON.h"
}

class minecraftRegion : public map{
    public:
        minecraftRegion(const char* path, cJSON* version);
        virtual ~minecraftRegion();
    private:

};

#endif
