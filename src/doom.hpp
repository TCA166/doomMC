#ifndef DOOM_HPP
#define DOOM_HPP

#include "weapons.hpp"

static const struct weapon fist = {true, 20, AMMO_NONE, 1};
static const struct weapon pistol = {true, 10, AMMO_CLIP, 12};
static const struct weapon shotgun = {false, 20, AMMO_SHELL, 6};
static const struct weapon superShotgun = {false, 40, AMMO_SHELL, 2};
static const struct weapon chaingun = {false, 5, AMMO_CLIP, 200};
static const struct weapon rocketLauncher = {false, 50, AMMO_ROCKET, 5};
static const struct weapon plasmaRifle = {false, 20, AMMO_CELL, 50};
static const struct weapon BFG9000 = {false, 100, AMMO_CELL, 1};
static const struct weapon chainsaw = {false, 50, AMMO_NONE, 1};

//DOOM weapons
static const struct weapon doomWeapons[9] = {fist, pistol, shotgun, superShotgun, chaingun, rocketLauncher, plasmaRifle, BFG9000, chainsaw};

static const struct ammo none = {0, 0};
static const struct ammo clip = {50, 200};
static const struct ammo shell = {10, 50};
static const struct ammo cell = {50, 250};
static const struct ammo rocket = {5, 50};

//DOOM ammo
static const struct ammo doomAmmunition[5] = {none, clip, shell, cell, rocket};

#endif
