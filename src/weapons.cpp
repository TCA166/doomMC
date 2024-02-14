#include "weapons.hpp"
#include <string.h>

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

byteArray weapon::getNBT() const{
    //FIXME causes the client to throw an error
    byteArray result;
    result.bytes = new byte[100];
    result.bytes[0] = TAG_COMPOUND;
    size_t offset = 1;
    offset += writeBigEndianShort(result.bytes + offset, 0);
    result.bytes[offset++] = TAG_COMPOUND;
    offset += writeNBTstring(result.bytes + offset, "display", 7);
    {
        result.bytes[offset++] = TAG_STRING;
        offset += writeNBTstring(result.bytes + offset, "Name", 4);
        size_t nameLen = strlen(this->name);
        offset += writeNBTstring(result.bytes + offset, this->name, nameLen);
    }
    {
        result.bytes[offset++] = TAG_LIST;
        offset += writeNBTstring(result.bytes + offset, "Lore", 4);
        result.bytes[offset++] = TAG_STRING;
        offset += writeBigEndianInt(result.bytes + offset, 3);
        offset += writeNBTstring(result.bytes + offset, "Max ammunition:", 15);
        offset += writeNBTstring(result.bytes + offset, "Rate of fire:", 13);
        offset += writeNBTstring(result.bytes + offset, "Damage:", 7);

    }
    result.bytes[offset++] = TAG_INVALID;
    result.bytes[offset++] = TAG_INVALID;
    result.len = offset;
    return result;
}
