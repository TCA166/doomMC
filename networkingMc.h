#include "mcTypes.h"

#define NO_COMPRESSION -1

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
#define packetNull(packet) packet.data == NULL && packet.size == -1 && packet.packetId == 0
//A packet that is considered to be NULL
#define nullPacket (packet){-1, 0, NULL}

/*!
 @brief Reads a packet from a socket
 @param socketFd the socket to read from
 @param compression the compression level of the server
 @return the packet read
*/
packet readPacket(int socketFd, int compression);
