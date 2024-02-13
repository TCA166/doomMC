#ifndef DOOM_HPP
#define DOOM_HPP

#include "weapons.hpp"

static const ammo none = ammo(0, 0, 20, 1);
static const ammo pistolAmmo = ammo(1, 200, 10, 100);
static const ammo shotgunAmmo = ammo(2, 50, 20, 100);
static const ammo chaingunAmmo = ammo(3, 200, 5, 100);
static const ammo rocketAmmo = ammo(4, 50, 50, 100);
static const ammo plasmaAmmo = ammo(5, 250, 20, 100);

static const weapon fist = weapon("Fist", 10, &none, 1);
static const weapon pistol = weapon("Pistol", 10, &pistolAmmo, 1);
static const weapon shotgun = weapon("Shotgun", 20, &shotgunAmmo, 1);
static const weapon superShotgun = weapon("Super Shotgun", 30, &shotgunAmmo, 1);
static const weapon chaingun = weapon("Chaingun", 5, &chaingunAmmo, 1);
static const weapon rocketLauncher = weapon("Rocket Launcher", 30, &rocketAmmo, 1);
static const weapon plasmaRifle = weapon("Plasma Rifle", 10, &plasmaAmmo, 1);
static const weapon BFG9000 = weapon("BFG 9000", 50, &plasmaAmmo, 1);
static const weapon chainsaw = weapon("Chainsaw", 5, &none, 1);

//DOOM weapons
static const weapon doomWeapons[9] = {fist, pistol, shotgun, superShotgun, chaingun, rocketLauncher, plasmaRifle, BFG9000, chainsaw};

static const bool initWeapons[MAX_WEAPONS] = {true, true, false, false, false, false, false, false, false};

static const uint8_t initAmmo[MAX_WEAPONS] = {0, 50, 10, 0, 0, 0, 0, 0, 0};

#endif
