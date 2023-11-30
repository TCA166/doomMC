#include "player.hpp"

player::player(server* server, int fd, state_t state, char* username, int compression, int32_t protocol) : client(server, fd, state, username, compression, protocol){
    
}
