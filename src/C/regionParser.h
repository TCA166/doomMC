#include "mcTypes.h"

//Authorship note: this code is mine, but from a different project. This is why this is also C and not C++. I don't want to rewrite it. https://github.com/TCA166/mcSavefileParsers/blob/master/regionParser.h

//How many chunks are in a region
#define chunkN 1024

//Length in bytes of savefile segment 
#define segmentLength 4096

//How to translate the compression byte

#define GZip 1
#define Zlib 2
#define Uncompressed 3

//If the chunk offset is 0 and sector count is 0 then the chunk is considered not generated and empty so basically NULL
#define chunkIsNull(chunk) (chunk.offset == 0 && chunk.sectorCount == 0)

#define coordsToIndex(x, z) ((x & 31) + (z & 31) * 32)

#define coordsToOffset(x, z) 4 * coordsToIndex(x, z)

//A struct representing a mc game chunk
typedef struct chunk{
    int x; //x coordinate in chunk coordinates
    int z; //z coordinate in chunk coordinates
    unsigned int offset; //3 bytes of offset
    byte sectorCount; //1 byte indicating chunk data length
    unsigned int timestamp;
    int byteLength; //it actually is signed in the files
    byte compression;
    byte* data; //pointer to decompressed bytes
} chunk;

/*!
 @brief gets the chunks from a region file
 @param regionFile the file to get the chunks from
 @return an array of chunks of length chunkN
*/
chunk* getChunks(FILE* regionFile);
