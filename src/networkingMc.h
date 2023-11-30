#include "mcTypes.h"
#include "packetDefinitions.h"

#ifndef NO_COMPRESSION

#define NO_COMPRESSION -1

#define NONE_STATE 0
#define STATUS_STATE 1
#define LOGIN_STATE 2
#define PLAY_STATE 3

//A minecraft style networking packet
typedef struct packet{
    int32_t size; //The size of data
    byte packetId;
    byte* data;
} packet;

/*!
 @brief Checks if packet is identical to nullPacket
 @param packet the packet to check
 @return true or false
*/
#define packetNull(packet) (packet.data == NULL && packet.size == -1 && packet.packetId == 0)
//A packet that is considered to be NULL
#define nullPacket (packet){-1, 0, NULL}

/*!
 @brief Reads a packet from a socket
 @param socketFd the socket to read from
 @param compression the compression level of the server
 @return the packet read
*/
packet readPacket(int socketFd, int compression);

/*! 
 @brief Sends the given packet to the Minecraft server in the correct format
 @param socketFd file descriptor that identifies a properly configured socket
 @param size the size of data
 @param packetId an id that identifies the packet in the Minecraft protocol
 @param data the values of fields of the packet
 @param compression the compression level established, or NO_COMPRESSION
 @return the number of bytes written, EOF or -1 for error
*/
ssize_t sendPacket(int socketFd, int size, int packetId, const byte* data, int compression);

#endif
