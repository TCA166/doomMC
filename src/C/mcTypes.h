#ifndef positionX

#include <stdbool.h>
#include <inttypes.h>
//NOTE: I tend to use types from inttypes when dealing with ints that play a part in the protocol and MUST have a set size. 
//However things like socketFds and indexes dont't need to be a set size so there I tend to use generic ints and shorts

#include "../../cNBT/nbt.h"

//Just look at all of these... peculiar types. position especially

typedef uint8_t byte;

//Type for Minecraft UUID which should be unsigned int128
typedef __uint128_t UUID_t;

typedef uint8_t angle_t;

//Minecraft identifier - a string in this format space:name
typedef char* identifier;

//Special minecraft type for storing position as an array of 2 int26 and 1 int12
typedef int64_t position;

//Macros

#define nullStringArray (stringArray){NULL, 0}

//VarInt and VarLong are encoded as strings of bytes, with each byte storing only 7 bits of data (SEGMENT_BITS) and one bit (CONTINUE_BIT) indicating whether the next byte also belongs to this varInt
//And since 32/7 = 4,5 MAX_VAR_INT is 5

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80
#define MAX_VAR_INT 5 //Maximum number of bytes a VarInt can take up
#define MAX_VAR_LONG 10 //Maximum number of bytes a VarLong can take up

//Gets the X from position
#define positionX(position) (position >> 38)

//Gets the Y from position
#define positionY(position) (position << 52 >> 52)

//Gets the Z from position
#define positionZ(position) (position << 26 >> 38)

//Converts a set of ints into a position type
#define toPosition(X, Y, Z) ((position)((int)(X) & 0x3FFFFFF) << 38) | ((position)((int)(Z) & 0x3FFFFFF) << 12) | (position)((int)(Y) & 0xFFF)

/*!
 @struct byteArray
 @brief An array with an attached size_t
 @param bytes the array itself
 @param len the length of the bytes array
*/
typedef struct byteArray{
    byte* bytes;
    size_t len;
} byteArray;

#define nullByteArray (byteArray){NULL, 0}

typedef struct stringArray{
    char** arr;
    size_t len;
} stringArray;

typedef struct identifierArray{
    identifier* arr;
    size_t len;
} identifierArray;

typedef struct slot{
    bool present;
    int32_t id;
    uint8_t count;
    nbt_node* NBT;
    int32_t cooldown;
} slot;

typedef struct palettedContainer{
    size_t paletteSize;
    int32_t* palette;
    int32_t* states;
} palettedContainer;

typedef struct bitSet{
    size_t length;
    int64_t* data;
} bitSet;

#define nullPalettedContainer (palettedContainer){0, NULL, NULL}

#define blockPaletteLowest 4
#define biomePaletteLowest 1

#define createLongMask(startBit, X) ((((uint64_t)1) << X) - 1) << startBit

/*! 
 @brief Writes the given value to the buffer as VarInt
 @param buff pointer to an allocated buffer where the VarInt bytes shall be written to
 @param value the value that will be written to the buffer
 @return the number of bytes written to buff
*/
size_t writeVarInt(byte* buff, int32_t value);

/*! 
 @brief Reads the VarInt from buffer at the given index
 @param buff the buffer from which to read the value
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the int encoded as VarInt in buff at index
*/
int32_t readVarInt(const byte* buff, int* index);

/*! 
 @brief Writes the given string to the buffer with a VarInt length preceding it
 @param buff the buffer that the string will be written to
 @param string the string that is to be written to buff
 @param stringLen the length of string
 @return the number of bytes written, including the size of VarInt in front of the string
*/
size_t writeString(byte* buff, const char* string, int stringLen);

/*!
 @brief Parses an encoded Minecraft string and writes it into a newly allocated buffer making it NULL terminated
 @param buff the buffer within which the string is encoded
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the pointer to the encoded string 
*/
char* readString(const byte* buff, int* index);

/*!
 @brief Writes a byteArray to the buffer
 @param buff the buffer to which the byteArray should be written to
 @param arr the byteArray that should be written
 @return the number of bytes written 
*/
size_t writeByteArray(byte* buff, byteArray arr);

/*!
 @brief Parses an encoded Minecraft byte array and writes it into a newly allocated buffer
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return a byte array
*/
byteArray readByteArray(const byte* buff, int* index);

/*!
 @brief Reads a little endian int32 from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded int32
*/
int32_t readInt(const byte* buff, int* index);

/*!
 @brief Writes a little endian as big endian int32 to the buffer
 @param buff the buffer to write to
 @param num the number to write
 @return the amount of bytes written
*/
size_t writeBigEndianInt(byte* buff, int32_t num);

/*!
 @brief Reads a big endian int32 from the buffer and then swaps the endianness
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded int32
*/
int32_t readBigEndianInt(const byte* buff, int* index);

/*!
 @brief Reads an int64 from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded int64
*/
int64_t readLong(const byte* buff, int* index);

/*!
 @brief Writes an int64 to the buffer as big endian
 @param buff the buffer to write to
 @param num the number to write
 @return the amount of bytes written
*/
size_t writeBigEndianLong(byte* buff, int64_t num);

/*!
 @brief Reads a big endian int64 from the buffer and then swaps the endianness
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded int64
*/
int64_t readBigEndianLong(const byte* buff, int* index);

/*!
 @brief Reads a big endian int64 from the buffer and then swaps the endianness without preserving the sign bit
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded uint64
*/
uint64_t readBigEndianULong(const byte* buff, int* index);

/*!
 @brief Read a boolean from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded boolean
*/
bool readBool(const byte* buff, int* index);

/*!
 @brief Reads a byte from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the byte
*/
byte readByte(const byte* buff, int* index);

/*!
 @brief Reads an array of strings from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return a stringArray object
*/
stringArray readStringArray(const byte* buff, int* index);

/*!
 @brief Writes a short to the buffer
 @param buff the buffer to write to
 @param num the number to write
 @return the amount of bytes written
*/
size_t writeShort(byte* buff, int16_t num);

/*!
 @brief Swaps the endianness and then writes a short to the buffer
 @param buff the buffer to write to
 @param num the number to write
 @return the amount of bytes written
*/
size_t writeBigEndianShort(byte* buff, int16_t num);

/*!
 @brief Swaps the endianness without preserving the sign bit and then writes a short to the buffer
 @param buff the buffer to write to
 @param num the number to write
 @return the amount of bytes written
*/
size_t writeBigEndianUShort(byte* buff, uint16_t num);

/*!
 @brief Reads a short from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded short
*/
int16_t readShort(const byte* buff, int* index);

/*!
 @brief Reads a short from the buffer at index and then swaps the endianness
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded short
*/
int16_t readBigEndianShort(const byte* buff, int* index);

/*!
 @brief Reads an unsigned short from the buffer at index and then swaps the endianness without preserving the position of the sign bit
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded short
*/
uint16_t readBigEndianUShort(const byte* buff, int* index);

/*!
 @brief Reads a UUID from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded UUID
*/
UUID_t readUUID(const byte* buff, int* index);

/*!
 @brief Reads a double from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded double
*/
double readDouble(const byte* buff, int* index);

/*!
 @brief Reads a double from the buffer at index and then swaps the endianness
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded double
*/
double readBigEndianDouble(const byte* buff, int* index);

/*!
 @brief Calculates the size of the nbt tag in the buffer
 @param buff the buffer that contains the nbt tag
 @param inCompound If the tag is contained withing a compound tag. false unless you know what you are doing
 @return the size of the nbt tag in bytes, or 0 for error
*/
size_t nbtSize(const byte* buff, bool inCompound);

/*!
 @brief Reads a slot type from the memory
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded slot
*/
slot readSlot(const byte* buff, int* index);

/*!
 @brief Reads a float from the memory
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded float
*/
float readFloat(const byte* buff, int* index);

/*!
 @brief Reads a float from memory and then swaps the endianness
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded float
*/
float readBigEndianFloat(const byte* buff, int* index);

/*!
 @brief Reads a varLong from memory
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded varLong
*/
int64_t readVarLong(const byte* buff, int* index);

/*!
 @brief Reads a palettedContainer from buffer
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @param bitsLowest the lowest acceptable size of elements in states
 @param bitsThreshold the threshold that determines whether bits per element in the states array should be determined dynamicly
 @param globalPaletteSize the size of the globalPallete the palette in this palettedContainer will point to
 @return palettedContainer or nullPalettedContainer on error
*/
palettedContainer readPalettedContainer(const byte* buff, int* index, const int bitsLowest, const int bitsThreshold, const size_t globalPaletteSize);

/*!
 @brief reads a java like bitset from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded bitSet
*/
bitSet readBitSet(const byte* buff, int* index);

/*!
 @brief compares the two byteArrays
 @return true if the two byteArrays are identical
*/
bool cmpByteArray(byteArray* a, byteArray* b);

/*!
 @brief Writes a slot to the buffer
 @param buff the buffer to write to 
 @param s the slot to write
 @return the amount of bytes written
*/
size_t writeSlot(byte* buff, slot* s);

/*!
 @brief Writes a float to the buffer
 @param buff the buffer to write to 
 @param f the float to write
 @return the amount of bytes written
*/
size_t writeBigEndianFloat(byte* buff, float f);

/*!
 @brief Writes a double to the buffer
 @param buff the buffer to write to
 @param d the double to write
 @return the amount of bytes written
*/
size_t writeBigEndianDouble(byte* buff, double d);

#endif
