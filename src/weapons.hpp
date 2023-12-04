#ifndef WEAPONS_HPP
#define WEAPONS_HPP

typedef enum{
    AMMO_CLIP=0,
    AMMO_SHELL=1,
    AMMO_CELL=2,
    AMMO_ROCKET=3,
    AMMO_NONE=-1
} ammo_t;

struct weapon{
    bool owned;
    int damage;
    ammo_t ammoId;
    unsigned int rateOfFire;
};

struct ammo{
    unsigned int count;
    unsigned int maxAmmo;
};

//DOOM weapons
static const struct weapon fist = {true, 20, AMMO_NONE, 1};
static const struct weapon pistol = {true, 10, AMMO_CLIP, 12};
static const struct weapon shotgun = {false, 20, AMMO_SHELL, 6};
static const struct weapon superShotgun = {false, 40, AMMO_SHELL, 2};
static const struct weapon chaingun = {false, 5, AMMO_CLIP, 200};
static const struct weapon rocketLauncher = {false, 50, AMMO_ROCKET, 5};
static const struct weapon plasmaRifle = {false, 20, AMMO_CELL, 50};
static const struct weapon BFG9000 = {false, 100, AMMO_CELL, 1};
static const struct weapon chainsaw = {false, 50, AMMO_NONE, 1};

static const struct weapon weapons[9] = {fist, pistol, shotgun, superShotgun, chaingun, rocketLauncher, plasmaRifle, BFG9000, chainsaw};

//DOOM ammo
static const struct ammo clip = {50, 200};
static const struct ammo shell = {10, 50};
static const struct ammo cell = {50, 300};
static const struct ammo rocket = {5, 50};

static const struct ammo ammunition[4] = {clip, shell, cell, rocket};

#endif
