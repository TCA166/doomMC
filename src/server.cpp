#include "server.hpp"
#include "player.hpp"
#include <iostream>
#include "map/udmf.hpp"
#include "map/mcr.hpp"
#include <spdlog/spdlog.h>

extern  "C"{
    #include <sys/socket.h>
    #include <unistd.h>
    #include <sys/random.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <string.h>
    #include "../cNBT/nbt.h"
    #include <sys/epoll.h>
    #include <dirent.h>
}

#define MAX_LISTEN 5

#define mapFolder "maps/"

static cJSON* readJson(const char* filename){
    FILE* statusFile = fopen(filename, "r");
    if(statusFile == NULL){
        perror("fopen");
        return NULL;
    }
    fseek(statusFile, 0, SEEK_END);
    long statusSize = ftell(statusFile);
    rewind(statusFile);
    char* statusJson = (char*)calloc(statusSize + 1, sizeof(char));
    if(fread(statusJson, 1, statusSize, statusFile) < (size_t)statusSize){
        throw std::error_code(errno, std::generic_category());
    }
    fclose(statusFile);
    cJSON* status = cJSON_ParseWithLength(statusJson, statusSize);
    if(status == NULL){
        perror("cJSON_Parse");
        return NULL;
    }
    return status;
}

static nbt_node* readNBT(const char* filename){
    FILE* codecFile = fopen(filename, "rb");
    if(codecFile == NULL){
        perror("fopen");
        return NULL;
    }
    fseek(codecFile, 0, SEEK_END);
    size_t codecSize = ftell(codecFile);
    rewind(codecFile);
    byte* codecData = (byte*)malloc(codecSize);
    if(fread(codecData, 1, codecSize, codecFile) < codecSize){
        throw std::error_code(errno, std::generic_category());
    }
    fclose(codecFile);
    nbt_node* codec = nbt_parse(codecData, codecSize);
    if(codec == NULL){
        perror("nbt_parse");
        return NULL;
    }
    return codec;
}

server::server(uint16_t port, unsigned int maxPlayers, unsigned int lobbyCount, unsigned int maxConnected, const char* statusFilename, const char* registryCodecFilename, const char* versionFilename) : lobbyCount(lobbyCount), maxConnected(maxConnected){
    {//create the master socket
        this->masterSocket = socket(AF_INET, SOCK_STREAM, 0);
        if(masterSocket < 0){
            throw std::error_code(errno, std::generic_category());
        }
        int opt = true;
        //set master socket to allow multiple connections
        if(setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 ){
            throw std::error_code(errno, std::generic_category());
        }
        //master socket address
        this->address = {AF_INET, htons(port), INADDR_ANY};
        this->address.sin_addr.s_addr = INADDR_ANY;
        this->addrLen = sizeof(this->address);
        //bind the master socket to localhost port
        if(bind(masterSocket, (struct sockaddr*)&this->address, this->addrLen) < 0){
            throw std::error_code(errno, std::generic_category());
        }
        //try to specify maximum of 5 pending connections for the master socket
        if(listen(masterSocket, MAX_LISTEN) < 0){
            throw std::error_code(errno, std::generic_category());
        }
    }
    {
        nbt_node* registryCodec = readNBT(registryCodecFilename);
        buffer codec = nbt_dump_binary(registryCodec);
        nbt_free(registryCodec);
        this->registryCodec = {codec.data, codec.len};
    }
    this->lobbies = new lobby*[lobbyCount];
    this->epollFd = epoll_create1(0);
    if(this->epollFd < 0){
        throw std::error_code(errno, std::generic_category());
    }
    {
        epoll_event masterEvent;
        masterEvent.events = EPOLLIN;
        masterEvent.data.fd = masterSocket;
        if(epoll_ctl(this->epollFd, EPOLL_CTL_ADD, masterSocket, &masterEvent) < 0){
            throw std::error_code(errno, std::generic_category());
        }
    }
    cJSON* version = readJson(versionFilename);
    DIR* dir = opendir(mapFolder);
    if(dir == NULL){
        spdlog::error("Could not open map folder {}", mapFolder);
        throw std::error_code(errno, std::generic_category());
    }
    for(unsigned int i = 0; i < lobbyCount; i++){
        dirent* ent = readdir(dir);
        if(ent == NULL){
            if(errno != 0){
                spdlog::error("Could not read map folder {}", mapFolder);
                throw std::error_code(errno, std::generic_category());
            }
            else{
                throw std::runtime_error("Not enough maps in map folder");
            }
        }
        if(ent->d_type == DT_REG){
            spdlog::debug("Found map {}", ent->d_name);
            map* newMap = NULL;
            while(newMap == NULL){
                //check if the extension is .mca
                if(strcmp(ent->d_name + strlen(ent->d_name) - 4, ".mca") == 0){
                    newMap = new minecraftRegion((mapFolder + std::string(ent->d_name)).c_str(), version);
                }
                //else if(strcmp(ent->d_name + strlen(ent->d_name) - 4, ".udm") == 0){
                //    newMap = new udmf((mapFolder + std::string(ent->d_name)).c_str());
                //}
                else{
                    ent = readdir(dir);
                    if(ent == NULL){
                        if(errno != 0){
                            spdlog::error("Could not read map folder {}", mapFolder);
                            throw std::error_code(errno, std::generic_category());
                        }
                        else{
                            throw std::runtime_error("Not enough maps in map folder");
                        }
                    }
                    continue;
                }
            }
            lobby* l = new lobby(maxPlayers, &this->registryCodec, newMap);
            this->lobbies[i] = l;
        }
        else{
            i--;
        }
    }
    closedir(dir);
    cJSON_free(version);
    this->connectedCount = 0;
    this->connected = new client*[maxConnected]();
    cJSON* status = readJson(statusFilename);
    cJSON* players = cJSON_GetObjectItemCaseSensitive(status, "players");
    cJSON* max = cJSON_GetObjectItemCaseSensitive(players, "max");
    cJSON_SetNumberValue(max, maxPlayers * lobbyCount);
    this->message = status;
    {
        FILE* tagsFile = fopen("tags.bin", "rb");
        if(tagsFile == NULL){
            throw std::error_code(errno, std::generic_category());
        }
        fseek(tagsFile, 0, SEEK_END);
        size_t tagsSize = ftell(tagsFile);
        rewind(tagsFile);
        byte* tagsData = (byte*)malloc(tagsSize);
        if(fread(tagsData, 1, tagsSize, tagsFile) < tagsSize){
            throw std::error_code(errno, std::generic_category());
        }
        fclose(tagsFile);
        this->tags = {tagsData, tagsSize};
    }
}

server::~server(){
    cJSON_free(this->message);
    delete[] this->lobbies;
    delete[] this->connected;
    close(this->masterSocket);
    close(this->epollFd);
    free(this->registryCodec.bytes);
}

int server::run(){
    epoll_event* events = new epoll_event[maxConnected];
    while(true){
        //wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        int activity = epoll_wait(this->epollFd, events, this->maxConnected, infiniteTime);
        if((activity < 0) && (errno != EINTR)){
            perror("select");
            return EXIT_FAILURE;
        }
        for(int i = 0; i < activity; i++){
            if(events[i].events & EPOLLRDHUP){
                client* c = (client*)events[i].data.ptr;
                this->removeClient(c->getIndex());
                delete c;
                continue;
            }
            if(events[i].data.fd == masterSocket){
                int newSocket;
                if((newSocket = accept(masterSocket, (struct sockaddr*)&this->address, (socklen_t*)&this->addrLen)) < 0){
                    perror("accept");
                    return EXIT_FAILURE;
                }
                //set newSocket to non blocking
                if(fcntl(newSocket, F_SETFL, fcntl(newSocket, F_GETFL, 0) | O_NONBLOCK) < 0){
                    perror("fcntl");
                    return EXIT_FAILURE;
                }
                spdlog::debug("New connection from {}", inet_ntoa(address.sin_addr));
                this->createClient(newSocket);
            }
            else if(events[i].data.ptr != NULL){
                client* c = (client*)events[i].data.ptr;
                packet p = c->getPacket();
                if(packetNull(p)){
                    spdlog::warn("Couldn't read packet from client {}({}). Reason:{}", c->getUUID(), c->getIndex(), strerror(errno));
                    continue;
                }
                c->handlePacket(&p);
                free(p.data);
                if((errno != EAGAIN && errno != EWOULDBLOCK && errno > 0) || c->getState() == PLAY_STATE){
                    spdlog::debug("Server losing track of client {}({})", c->getUUID(), c->getIndex());
                    this->removeClient(c->getIndex());
                    delete c;
                }
            }
        }
    }
    delete[] events;
    return EXIT_SUCCESS;
}

unsigned int server::getLobbyCount() const{
    return this->lobbyCount;
}

unsigned int server::getPlayerCount() const{
    unsigned int playerCount = 0;
    for(unsigned int i = 0; i < this->lobbyCount; i++){
        playerCount += this->lobbies[i]->getPlayerCount();
    }
    return playerCount;
}

void server::createClient(int socket){
    if(this->connectedCount >= this->maxConnected){
        spdlog::warn("Server full");
        close(socket);
        return;
    }
    for(unsigned int i = 0; i < this->maxConnected; i++){
        if(this->getClient(i) == NULL){
            this->connected[i] = new client(this, socket, i);
            //add newSocket to epoll
            epoll_event event;
            event.events = EPOLLIN | EPOLLRDHUP;
            event.data.ptr = this->connected[i];
            epoll_ctl(this->epollFd, EPOLL_CTL_ADD, socket, &event);
            spdlog::debug("Added client {}({}) to server", this->connected[i]->getUUID(), this->connected[i]->getIndex());
            break;
        }
    }
    connectedCount++;
}

void server::removeClient(int n){
    client* c = this->connected[n];
    if(c != NULL){
        this->connected[n] = NULL;
        this->connectedCount--;
        if(epoll_ctl(this->epollFd, EPOLL_CTL_DEL, c->getFd(), NULL) < 0){
            perror("epoll_ctl");
        }
    }
    else{
        spdlog::warn("Attempted to remove null client {}", n);
    }
}

client* server::getClient(int n) const{
    return this->connected[n];
}

cJSON* server::getMessage() const{
    cJSON* players = cJSON_GetObjectItemCaseSensitive(this->message, "players");
    cJSON* online = cJSON_GetObjectItemCaseSensitive(players, "online");
    cJSON_SetIntValue(online, this->getPlayerCount());
    return (cJSON*)this->message;
}

void server::addToLobby(client* c){
    for(unsigned int i = 0; i < this->lobbyCount; i++){
        if(this->lobbies[i]->getPlayerCount() < this->lobbies[i]->getMaxPlayers()){
            player* p = c->toPlayer();
            spdlog::debug("Adding client {}({}) to lobby {}", p->getUUID(), p->getIndex(), i);
            this->lobbies[i]->addPlayer(p);
            return;
        }
    }
    //TODO what do do with the client if there are no lobbies with space
}

const byteArray* server::getRegistryCodec() const{
    return &this->registryCodec;
}

const byteArray* server::getTags() const{
    return &this->tags;
}
