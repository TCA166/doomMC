#ifndef PLAYER_HPP

#define PLAYER_HPP

class player{
    public :
        const unsigned int id;
        player(unsigned int id, int socket);
        void update();
    private:
        const int socket;
};

#endif
