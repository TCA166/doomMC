#ifndef CLIENT_HPP

#define CLIENT_HPP

extern "C" {
    #include "mcTypes.h"
    #include "networkingMc.h"
}

typedef enum{
    NONE_STATE = 0,
    STATUS_STATE = 1,
    LOGIN_STATE = 2,
    PLAY_STATE = 3,
    COFNIG_STATE = 4
} state_t;

class server;

class client{
    protected:
        int fd;
        state_t state;
        char* username;
        int compression;
        int32_t protocol;
    private:
        void disconnect();
        server* serv;
    public:
        /*!
         @brief Constructs a client instance
         @param fd the socket file descriptor associated with the client
         @param state the state of the client
         @param username the username of the client
         @param compression the compression level established with the client
        */
        client(server* server, int fd, state_t state, char* username, int compression, int32_t protocol);
        client(server* server, int fd);
        /*!
         @brief Constructs a client instance with all values set to 0 or NULL
        */
        client();
        ~client();
        /*!
         @brief Gets the socket file descriptor associated with the client
         @return the socket file descriptor
        */
        int getFd();
        /*!
         @brief Gets the state of the client
         @return the state of the client
        */
        byte getState();
        /*!
         @brief Gets the username of the client
         @return the username of the client
        */
        char* getUsername();
        /*!
         @brief Gets the compression level established with the client
         @return the compression level
        */
        int getCompression();
        /*!
         @brief Sets the username of the client
         @param username the username to set
        */
        void setUsername(char* username);
        /*!
         @brief Handles a provided packet in reference to this instance
         @return 1 if the packet was handled successfully, 0 on EOF, -1 if an error occurred
        */
        int handlePacket(packet* p);
        /*!
        @brief Gets the packet from the associated file descriptor
        @return the packet or a null packet if a packet could not be read
        */
        packet getPacket();
        /*!
         @brief Sends a packet to the associated file descriptor
         @param data the data to send
         @param length the length of the data
         @param packetId the id of the packet
         @return the number of bytes sent, 0 on EOF or -1 if an error occurred
        */
        int send(byte* data, int length, byte packetId);
};

#endif
