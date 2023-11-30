extern "C" {
    #include "mcTypes.h"
    #include "networkingMc.h"
}

typedef enum{
    NONE_STATE = 0,
    STATUS_STATE = 1,
    LOGIN_STATE = 2,
    PLAY_STATE = 3
} state_t;

class client{
    protected:
        int fd;
        state_t state;
        char* username;
        int compression;
    private:
        void disconnect();
    public:
        /*!
         @brief Constructs a client instance
         @param fd the socket file descriptor associated with the client
         @param state the state of the client
         @param username the username of the client
         @param compression the compression level established with the client
        */
        client(int fd, state_t state, char* username, int compression);
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
         @return the number of bytes sent, 0 on EOF or -1 if an error occurred
        */
        int send(byte* data, int length, byte packetId);
};