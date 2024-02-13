#include "weapons.hpp"

ammo::ammo(uint8_t ammoId, uint8_t maxAmmo, uint8_t damage, uint16_t range) : ammoId(ammoId), maxAmmo(maxAmmo), damage(damage), range(range){

}

uint8_t ammo::getAmmoId() const{
    return this->ammoId;
}

void ammo::fire(){
    
}

void ammo::hit() const{

}

weapon::weapon(const char* name, unsigned int rateOfFire, const ammo* ammunition, unsigned int itemId) : ammunition(ammunition), rateOfFire(rateOfFire), name(name), itemId(itemId){

}

unsigned int weapon::getItemId() const{
    return this->itemId;
}

const ammo* weapon::getAmmunition() const{
    return this->ammunition;
}

unsigned int weapon::getRateOfFire() const{
    return this->rateOfFire;
}

const char* weapon::getName() const{
    return this->name;
}


