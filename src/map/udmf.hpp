#ifndef UDMF_HPP
#define UDMF_HPP

#include "map.hpp"

class udmf : public map{
    public:
        udmf(const char* path);
        virtual ~udmf();
    private:

};

#endif
