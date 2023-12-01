#ifndef PLAYER_HPP

#define PLAYER_HPP

#include "client.hpp"

class lobby;

class player : public client{
    public :
        player(server* server, int fd, state_t state, char* username, int compression, int32_t protocol);
        int handlePacket(packet* p);
        void setWeapons(int damage[9], int maxAmmo[9], int rateOfFire[9]);
        void setAmmo(int ammo[9]);
        void setHealth(int health);
        void setLocation(double x, double y, double z);
        void dealDamage(int damage);
        void sendMessage(char* message);
        void changeLobby(lobby* lobby);
        void startPlay(int32_t eid);
        int heldSlot;
    private:
        double x, y, z;
        bool onGround;
        float yaw, pitch;
        int health;
        int damage[9];
        int ammo[9];
        int maxAmmo[9];
        int rateOfFire[9];
        lobby* currentLobby;
        int32_t eid;
};

#endif
