#ifndef PLAYER_HPP

#define PLAYER_HPP

#include "client.hpp"

class player : public client{
    public :
        player(server* server, int fd, state_t state, char* username, int compression, int32_t protocol);
    private:
        
};

#endif
