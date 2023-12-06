#ifndef UDMF_HPP
#define UDMF_HPP

#include "map.hpp"

class udmf : private map{
    public:
        udmf(const char* path);
        virtual ~udmf();
    private:

};

#endif
