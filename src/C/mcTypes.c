#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <endian.h>
#include <math.h>

#include "mcTypes.h"

//If index is null then changes index to point to a newly created int with value of 0
#define getIndex(index) \
    int locIndex = 0; \
    if(index == NULL){ \
        index = &locIndex; \
    }

//the endian.h functions don't seem to work well on floating point values so these are here optimised as much as possible

inline static double swapDouble(double val){
    double retVal = 0;
    byte* restrict src = (byte*)&val;
    byte* restrict dest = (byte*)&retVal;
    dest[0] = src[7];
    dest[1] = src[6];
    dest[2] = src[5];
    dest[3] = src[4];
    dest[4] = src[3];
    dest[5] = src[2];
    dest[6] = src[1];
    dest[7] = src[0];
    return retVal;
}

inline static float swapFloat(float val){
    float retVal = 0;
    byte* restrict src = (byte*)&val;
    byte* restrict dest = (byte*)&retVal;
    dest[0] = src[3];
    dest[1] = src[2];
    dest[2] = src[1];
    dest[3] = src[0];
    return retVal;
}

//Gets the size of the nbt tag in buffer, assumes the nbt tag is of the given type
static inline size_t tagSize(const byte* buff, byte type);

size_t writeVarInt(byte* buff, int32_t value){
    short i = 0;
    while(true){
        if((value & ~SEGMENT_BITS) == 0){
            buff[i] = value;
            return i + 1;
        }
        buff[i] = (value & SEGMENT_BITS) | CONTINUE_BIT;
        value = (uint32_t)value >> 7;
        i++;
    }
}

int32_t readVarInt(const byte* buff, int* index){
    int32_t value = 0;
    int position = 0; //position in bits
    getIndex(index) //position in bytes
    const int startIndex = *index;
    while(true){
        byte currentByte = buff[*index];
        *index += 1;
        value |= (currentByte & SEGMENT_BITS) << position;

        if((currentByte & CONTINUE_BIT) == 0) break;

        position += 7;

        if(position >= 32 || (*index - startIndex) > MAX_VAR_INT){
            errno = EOVERFLOW;
            //I am torn on either using 0 or INT_MAX, well I guess at least INT_MAX is less likely to get returned and is more accurate % wise?
            return INT32_MAX; 
        }
    }
    return value;
}

size_t writeString(byte* buff, const char* string, int stringLen){
    for(int i = stringLen - 1; i >= 0; i--){
        if(string[i] == '\0'){
            stringLen--;
        }
    }
    size_t offset = writeVarInt(buff, stringLen);
    memcpy(buff + offset, string, stringLen);
    return stringLen + offset;
}

char* readString(const byte* buff, int* index){
    getIndex(index)
    int msgLen = readVarInt(buff, index);
    char* result = calloc(msgLen + 1, sizeof(char));
    memcpy(result, buff + *index, msgLen);
    *index += msgLen;
    return result;
}

size_t writeByteArray(byte* buff, byteArray arr){
    size_t offset = writeVarInt(buff, arr.len);
    memcpy(buff + offset, arr.bytes, arr.len);
    return offset + arr.len;
}

byteArray readByteArray(const byte* buff, int* index){
    getIndex(index)
    byteArray arr = nullByteArray;
    int arrLen = readVarInt(buff, index);
    arr.len = arrLen;
    byte* val = calloc(arrLen, sizeof(byte));
    memcpy(val, buff + *index, arrLen);
    arr.bytes = val;
    *index += arrLen;
    return arr;
}

int32_t readInt(const byte* buff, int* index){
    getIndex(index)
    int32_t result = *(int32_t*)(buff + *index);
    *index += sizeof(int32_t);
    return result;
}

int64_t readLong(const byte* buff, int* index){
    getIndex(index);
    int64_t result = *(int64_t*)(buff + *index);
    *index += sizeof(int64_t);
    return result;
}

bool readBool(const byte* buff, int* index){
    getIndex(index)
    bool result =  *(bool*)(buff + *index);
    *index += sizeof(bool);
    return result;
}

byte readByte(const byte* buff, int* index){
    getIndex(index)
    byte result = buff[*index];
    *index += 1;
    return result;
}

stringArray readStringArray(const byte* buff, int* index){
    stringArray result = nullStringArray;
    getIndex(index)
    int number = readVarInt(buff, index);
    result.len = number;
    result.arr = calloc(number, sizeof(char*));
    for(int i = 0; i < number; i++){
        result.arr[i] = readString(buff, index);
    }
    return result;
}

int16_t readShort(const byte* buff, int* index){
    getIndex(index)
    int16_t result = *(int16_t*)(buff + *index);
    *index += sizeof(int16_t);
    return result;
}

int16_t readBigEndianShort(const byte* buff, int* index){
    int16_t s = readShort(buff, index);
    return be16toh(s);
}

size_t writeBigEndianInt(byte* buff, int32_t num){
    int32_t bigEndian = htobe32(num);
    *(int32_t*)buff = bigEndian;
    return sizeof(int32_t);
}

int32_t readBigEndianInt(const byte* buff, int* index){
    int32_t i = readInt(buff, index);
    return be32toh(i);
}

UUID_t readUUID(const byte* buff, int* index){
    getIndex(index)
    UUID_t result;
    memcpy(&result, buff + *index, sizeof(UUID_t));
    *index += sizeof(UUID_t);
    return result; //UUIDs are stored as is. No need to swap
}

double readDouble(const byte* buff, int* index){
    getIndex(index)
    double result = *(double*)(buff + *index);
    *index += sizeof(double);
    return result;
}

size_t nbtSize(const byte* buff, bool inCompound){
    int res = 0;
    //each nbt tag starts with a byte indicating the type
    byte type = readByte(buff, &res);
    if(type > TAG_LONG_ARRAY || type == TAG_INVALID){
        return 0; //something went very wrong
    }
    //and then tags other than TAG_END if in compound tag have always 2 bytes indicating name length
    if(inCompound || type == TAG_COMPOUND){
        res += readBigEndianShort(buff, &res); //by doing this trick we increment res twice. Once by passing the pointer(by short size) and once by incrementing by returned value
    }
    //Then we just continue parsing
    res += tagSize(buff + res, type);
    return res;
}

static inline size_t tagSize(const byte* buff, byte type){
    int res = 0;
    switch(type){
        case TAG_INVALID:;
            return 0; //we return 0 to indicate INVALID
            break;
        case TAG_BYTE:;
            res += sizeof(byte);
            break;
        case TAG_SHORT:;
            res += sizeof(int16_t);
            break;
        case TAG_INT:;
            res += sizeof(int32_t);
            break;
        case TAG_LONG:;
            res += sizeof(int64_t);
            break;
        case TAG_FLOAT:;
            res += sizeof(float);
            break;
        case TAG_DOUBLE:;
            res += sizeof(double);
            break;
        case TAG_BYTE_ARRAY:;
            res += readBigEndianInt(buff, &res);
            break;
        case TAG_STRING:;
            res += (uint16_t)readBigEndianShort(buff, &res);
            break;
        case TAG_LIST:;
            //each list element has the same type
            byte type = readByte(buff, &res);
            int32_t num = readBigEndianInt(buff, &res);
            if(num <= 0){ //if the list is empty we need to increment the size
                //res += 1; //for whatever reason empty lists AREN'T FULLY EMPTY
                //AND THE WORST PART IS I CANNOT DETECT REALISTICALLY HOW MUCH EMPTY BYTES ARE APPENDED 
            }
            else{
                //else we parse each element in list (why are arrays named lists here?)
                while(num > 0){
                    res += tagSize(buff + res, type);
                    num--;
                }
            }
            break;
        case TAG_COMPOUND:;
            while(true){
                const byte* el = buff + res;
                size_t r = nbtSize(el, true);
                if(r == 0){
                    res++;
                    break;
                }
                else{
                    //fprintf(stderr, "%d ", r);
                    res += r;
                }
            }
            break;
        case TAG_INT_ARRAY:;
            res += readBigEndianInt(buff, &res) * sizeof(int32_t);
            break;
        case TAG_LONG_ARRAY:;
            res += readBigEndianInt(buff, &res) * sizeof(int64_t);
            break;
    }
    return res;
}

slot readSlot(const byte* buff, int* index){
    getIndex(index)
    slot result;
    result.present = readBool(buff, index);
    if(result.present){
        result.id = readVarInt(buff, index);
        result.count = readByte(buff, index);
        size_t sz = nbtSize(buff + *index, false);
        result.NBT = nbt_parse(buff + *index, sz);
        *index += sz;
    }
    else{
        result.id = 0;
        result.count = 0;
        result.NBT = NULL;
    }
    return result;
}

float readFloat(const byte* buff, int* index){
    getIndex(index)
    float result = *(float*)buff;
    *index += sizeof(float);
    return result;
}

int64_t readVarLong(const byte* buff, int* index){
    int64_t value = 0;
    int position = 0;
    getIndex(index)
    const int startIndex = *index;
    while(true){
        byte currentByte = buff[*index];
        *index += 1;
        value |= (int64_t)(currentByte & SEGMENT_BITS) << position;

        if((currentByte & CONTINUE_BIT) == 0) break;

        position += 7;

        if(position >= 64 || (*index - startIndex) > MAX_VAR_LONG){
            errno = EOVERFLOW;
            return INT_FAST64_MAX;
        }
    }
    return value;
}

size_t writeBigEndianLong(byte* buff, int64_t num){
    int64_t bigEndian = htobe64(num);
    *(int64_t*)buff = bigEndian;
    return sizeof(int64_t);
}

int64_t readBigEndianLong(const byte* buff, int* index){
    int64_t littleEndian = readLong(buff, index);
    return be64toh(littleEndian);
}

float readBigEndianFloat(const byte* buff, int* index){
    return swapFloat(readFloat(buff, index));
}

double readBigEndianDouble(const byte* buff, int* index){
    return swapDouble(readDouble(buff, index));
}

size_t writeShort(byte* buff, int16_t num){
    *(int16_t*)buff = num;
    return sizeof(int16_t);
}

size_t writeBigEndianShort(byte* buff, int16_t num){
    int16_t bigEndian = be16toh(num);
    return writeShort(buff, bigEndian);
}

uint64_t readBigEndianULong(const byte* buff, int* index){
    uint64_t littleEndian = readLong(buff, index);
    return be64toh(littleEndian);
}

palettedContainer readPalettedContainer(const byte* buff, int* index, const int bitsLowest, const int bitsThreshold, const size_t globalPaletteSize){
    palettedContainer result;
    byte bitsPerEntry = readByte(buff, index);
    if(bitsPerEntry == 0){
        result.paletteSize = 1;
        result.palette = malloc(sizeof(int32_t));
        *result.palette = readVarInt(buff, index);
        result.states = NULL;
        (void)readVarInt(buff, index);
        return result;
    }
    else{
        if(bitsPerEntry >= bitsThreshold){
            bitsPerEntry = (byte)ceilf(log2f((float)globalPaletteSize));
            result.palette = NULL;
            result.paletteSize = 0;
        }
        else{
            if(bitsPerEntry < bitsLowest){
                bitsPerEntry = bitsLowest;
            }
            result.paletteSize = readVarInt(buff, index);
            result.palette = calloc(result.paletteSize, sizeof(uint32_t));
            for(int n = 0; n < result.paletteSize; n++){
                //this is a state index
                uint32_t element = readVarInt(buff, index);
                //sanity check 1
                if(element > globalPaletteSize){
                    errno = E2BIG; 
                    free(result.palette);
                    return nullPalettedContainer;
                }
                result.palette[n] = element;
            }
        }
        //and now we need to get the states
        const uint16_t numPerLong = (uint16_t)(64/bitsPerEntry); //number of elements of the array in a single long
        uint16_t arrIndex = 0; //the current MAIN array index
        const int32_t numLongs = readVarInt(buff, index); //the number of longs the MAIN array has been split into
        const size_t statesSize = (size_t)ceilf((float)numPerLong * (float)numLongs);
        result.states = calloc(statesSize, sizeof(uint32_t)); //the states
        for(int l = 0; l < numLongs; l++){//foreach Long
            uint64_t ourLong = readBigEndianULong(buff, index);
            for(uint8_t b = 0; b < numPerLong; b++){ //foreach element in long
                if(arrIndex >= statesSize){
                    break;
                }
                uint16_t bits = b * bitsPerEntry;
                uint32_t state = (uint32_t)((createLongMask(bits, bitsPerEntry) & ourLong) >> bits);
                //sanity check 2
                if((result.palette != NULL && state > result.paletteSize) || state > globalPaletteSize){
                    errno = E2BIG;
                    free(result.palette);
                    free(result.states);
                    return nullPalettedContainer;
                }
                result.states[arrIndex] = state;
                arrIndex++;
            }
        }
    }
    return result;
}

/*!
 @brief Writes an int to a Long at the given offset
 @param buff the long to write to
 @param value the value to write
 @param offset the offset at which to write the value
 @param bitsPerEntry the amount of bits the value can take
*/
static size_t writeIntToPackedArray(uint64_t* buff, int value, size_t offset, uint8_t bitsPerEntry){
    uint64_t mask = ((1ULL << bitsPerEntry) - 1) << offset;
    uint64_t maskedValue = (uint64_t)(value & ((1 << bitsPerEntry) - 1));
    *buff = (*buff & ~mask) | (maskedValue << offset);
    return offset + bitsPerEntry;
}

uint64_t writePackedLong(const int32_t* val, int perLong, int index, uint8_t bpe){
    uint64_t l = 0;
    for(int n = 0; n < perLong; n++){
        writeIntToPackedArray(&l, val[index * perLong + n], n * bpe, bpe);
    }
    return l;
}

byteArray writePackedArray(int32_t* val, size_t len, uint8_t bpe, bool writeLength){
    byteArray result = nullByteArray;
    const int perLong = (int)(64 / bpe);
    const size_t longCount = (size_t)ceilf((float)len / (float)perLong);
    result.bytes = malloc((longCount * sizeof(int64_t)) + MAX_VAR_INT);
    if(writeLength){
        result.len += writeVarInt(result.bytes + result.len, longCount);
    }
    for(int i = 0; i < longCount; i++){
        uint64_t l = 0;
        for(int n = 0; n < perLong; n++){
            if(i * perLong + n >= len){
                break;
            }
            writeIntToPackedArray(&l, val[i * perLong + n], n * bpe, bpe);
        }
        result.len += writeBigEndianLong(result.bytes + result.len, l);
    }
    return result;
}

byteArray writePalletedContainer(palettedContainer* container, size_t globalPaletteSize){
    byteArray result = nullByteArray;
    switch(container->paletteSize){
        case 1:{ //pallette is single valued
            result.bytes = malloc(1 + (MAX_VAR_INT * 2));
            result.bytes[result.len] = 0;
            result.len++;
            result.len += writeVarInt(result.bytes + result.len, container->palette[0]);
            result.len += writeVarInt(result.bytes + result.len, 0);
            break;
        }
        default:{
            if(container->paletteSize <= 255){
                result.bytes = malloc(1 + (MAX_VAR_INT * container->paletteSize));
                byte bpe = (byte)ceilf(log2f((float)container->paletteSize));
                if(bpe < 4){
                    bpe = 4;
                }
                result.bytes[result.len] = bpe;
                result.len++;
                result.len += writeVarInt(result.bytes + result.len, container->paletteSize);
                for(int i = 0; i < container->paletteSize; i++){
                    result.len += writeVarInt(result.bytes + result.len, container->palette[i]);
                }
                byteArray states = writePackedArray(container->states, 4096, bpe, true);
                result.bytes = realloc(result.bytes, 1 + states.len + result.len);
                memcpy(result.bytes + result.len, states.bytes, states.len);
                free(states.bytes);
                result.len += states.len;
                break;
            }   
            //we move on to case 0 which means writing the data
        }
        case 0:{ //pallete is empty, the global pallete does everything
            byte bpe = (byte)ceilf(log2f((float)globalPaletteSize));
            byteArray states = writePackedArray(container->states, 4096, bpe, true);
            result.bytes = malloc(1 + states.len);
            result.bytes[result.len] = bpe;
            result.len++;
            memcpy(result.bytes + result.len, states.bytes, states.len);
            free(states.bytes);
            result.len += states.len;
            break;
        }
    }
    return result;
}

bitSet readBitSet(const byte* buff, int* index){
    getIndex(index)
    bitSet result;
    result.length = readVarInt(buff, index);
    result.data = calloc(result.length, sizeof(int64_t));
    for(int i = 0; i < result.length; i++){
        result.data[i] = readBigEndianLong(buff, index);
    }
    return result;
}

size_t writeBitSet(byte* buff, const bitSet* set){
    size_t offset = writeVarInt(buff, set->length);
    for(int i = 0; i < set->length; i++){
        offset += writeBigEndianLong(buff + offset, set->data[i]);
    }
    return offset;
}

bool cmpByteArray(byteArray* a, byteArray* b){
    if(a->len != b->len){
        return false;
    }
    return memcmp(a->bytes, b->bytes, a->len) == 0;
}

size_t writeSlot(byte* buff, slot* s){
    size_t offset = 0;
    buff[offset] = s->present;
    offset++;
    if(s->present){
        offset += writeVarInt(buff + offset, s->id);
        buff[offset] = s->count;
        offset++;
        if(s->NBT != NULL){
            struct buffer b = nbt_dump_binary(s->NBT);
            memcpy(buff + offset, b.data, b.len);
            offset += b.len;
        }
        else{
            buff[offset] = TAG_INVALID;
            offset++;
        }
    }
    return offset;
}

size_t writeBigEndianFloat(byte* buff, float f){
    *(float*)buff = swapFloat(f);
    return sizeof(float);
}

size_t writeBigEndianDouble(byte* buff, double d){
    *(double*)buff = swapDouble(d);
    return sizeof(double);
}

angle_t toAngle(float f){
    int angle = abs((int)f); //f can be negative, it changes nothing
    angle %= 360; //f can be greater than 360, we need to fix that
    return (angle_t)(f * 256 / 360);
}

byteArray writeSections(palettedContainer* sections, palettedContainer* biomes, size_t sectionCount, size_t globalPaletteSize){
    size_t offset = 0;
    byte* data = malloc(2);
    for(int i = 0; i < sectionCount; i++){
        palettedContainer* section = sections + i;
        int single[1] = {0};
        palettedContainer emptyBiomes = {1, single, NULL};
        palettedContainer* biome = &emptyBiomes;
        if(biomes != NULL){
            biome = biomes + i;
        }
        int16_t nonAir = 0;
        if(section->paletteSize == 1){
            nonAir = (section->palette[0] == 0) * 4096;
        }
        else{
            for(int j = 0; j < 4096; j++){
                if(section->states[j] != 0){
                    nonAir++;
                }
            }
        }
        offset += writeBigEndianShort(data + offset, nonAir);
        byteArray blocks = writePalletedContainer(section, globalPaletteSize);
        byteArray biomes = writePalletedContainer(biome, globalPaletteSize);
        data = (byte*)realloc(data, offset + blocks.len + biomes.len + 2);
        memcpy(data + offset, blocks.bytes, blocks.len);
        offset += blocks.len;
        free(blocks.bytes);
        memcpy(data + offset, biomes.bytes, biomes.len);
        offset += biomes.len;
        free(biomes.bytes);
    }
    return (byteArray){data, offset};
}
