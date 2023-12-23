#ifndef WEAPONS_HPP
#define WEAPONS_HPP

extern "C"{
    #include <stdint.h>
}

#define MAX_WEAPONS 9

typedef enum{
    AMMO_NONE=0,
    AMMO_CLIP=1,
    AMMO_SHELL=2,
    AMMO_CELL=3,
    AMMO_ROCKET=4
} ammo_t;

struct weapon{
    bool owned;
    int damage;
    ammo_t ammoId;
    unsigned int rateOfFire;
};

struct ammo{
    uint8_t count;
    uint8_t maxAmmo;
};

#endif
