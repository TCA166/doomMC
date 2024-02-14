#ifndef WEAPONS_HPP
#define WEAPONS_HPP

class player;

extern "C"{
    #include <stdint.h>
    #include "C/mcTypes.h"
}

#define MAX_WEAPONS 9
#define MELEE_AMMO_ID 0

class ammo{
    private:
        const uint8_t ammoId;
        const uint8_t maxAmmo;
        const uint8_t damage;
        const uint16_t range;
    public:
        ammo(uint8_t ammoId, uint8_t maxAmmo, uint8_t damage, uint16_t range);
        uint8_t getAmmoId() const;
        void fire();
        void hit() const;
};

class weapon{
    private:
        const ammo* ammunition;
        const unsigned int rateOfFire;
        const char* name;
        const unsigned int itemId;
    public:
        weapon(const char* name, unsigned int rateOfFire, const ammo* ammunition, unsigned int itemId);
        void fire(player* p);
        unsigned int getRateOfFire() const;
        const char* getName() const;
        const ammo* getAmmunition() const;
        unsigned int getItemId() const;
        byteArray getNBT() const;
};

#endif
