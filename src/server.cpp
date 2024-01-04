#include "server.hpp"
#include "player.hpp"
#include <iostream>
#include "map/udmf.hpp"
#include "map/mcr.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

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

#define PORT 8080
#define MAX_LISTEN 5

#define MAX_LOBBIES 1

#define MAX_CLIENTS 100

#define templateRegistry "registryCodec.nbt"

#define mapFolder "maps/"

server::server(unsigned long maxPlayers, unsigned long lobbyCount, unsigned long maxConnected, cJSON* message, nbt_node* registryCodec, int epollFd) : lobbyCount(lobbyCount), maxConnected(maxConnected), epollFd(epollFd){
    buffer codec = nbt_dump_binary(registryCodec);
    this->registryCodec = {codec.data, codec.len};
    this->lobbies = new lobby*[lobbyCount];
    for(int i = 0; i < lobbyCount; i++){
        DIR* dir = opendir(mapFolder);
        if(dir == NULL){
            perror("opendir");
            throw "Could not open map folder";
        }
        dirent* ent;
        while((ent = readdir(dir)) != NULL){
            if(ent->d_type == DT_REG){
                spdlog::debug("Found map {}", ent->d_name);
                map* newMap;
                //check if the extension is .mcr
                if(strcmp(ent->d_name + strlen(ent->d_name) - 4, ".mcr") == 0){
                    newMap = new minecraftRegion((mapFolder + std::string(ent->d_name)).c_str());
                }
                else if(strcmp(ent->d_name + strlen(ent->d_name) - 4, ".udm") == 0){
                    newMap = new udmf((mapFolder + std::string(ent->d_name)).c_str());
                }
                lobby* l = new lobby(maxPlayers, &this->registryCodec, newMap);
                this->lobbies[i] = l;
            }
        }
    }
    this->connectedCount = 0;
    this->connected = (client**)calloc(maxConnected, sizeof(client*));
    this->message = message;
}

unsigned long server::getLobbyCount(){
    return this->lobbyCount;
}

unsigned long server::getPlayerCount(){
    unsigned long playerCount = 0;
    for(int i = 0; i < this->lobbyCount; i++){
        playerCount += this->lobbies[i]->getPlayerCount();
    }
    return playerCount;
}

void server::createClient(int socket){
    if(this->connectedCount >= this->maxConnected){
        close(socket);
        return;
    }
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(this->getClient(i) == NULL){
            this->connected[i] = new client(this, socket, i);
            //add newSocket to epoll
            epoll_event event;
            event.events = EPOLLIN | EPOLLRDHUP;
            event.data.ptr = this->connected[i];
            epoll_ctl(this->epollFd, EPOLL_CTL_ADD, socket, &event);
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

client* server::getClient(int n){
    return this->connected[n];
}

cJSON* server::getMessage(){
    return (cJSON*)this->message;
}

void server::addToLobby(client* c){
    for(int i = 0; i < this->lobbyCount; i++){
        if(this->lobbies[i]->getPlayerCount() < this->lobbies[i]->getMaxPlayers()){
            player* p = c->toPlayer();
            spdlog::debug("Adding client {}({}) to lobby {}", p->getUUID(), p->getIndex(), i);
            this->lobbies[i]->addPlayer(p);
            return;
        }
    }
    //TODO what do do with the client if there are no lobbies with space
}

const byteArray* server::getRegistryCodec(){
    return &this->registryCodec;
}

int main(int argc, char *argv[]){
    //socket that accepts new connections
    int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(masterSocket < 0){
        perror("socket");
        return EXIT_FAILURE;
    }
    int opt = true;
    //set master socket to allow multiple connections
    if(setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 ){
        perror("setsockopt");
        return EXIT_FAILURE;
    }
    //master socket address
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };
    address.sin_addr.s_addr = INADDR_ANY;
    size_t addrlen = sizeof(address);
    //bind the master socket to localhost port 8080
    if(bind(masterSocket, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind");
        return EXIT_FAILURE;
    }
    //try to specify maximum of 5 pending connections for the master socket
    if(listen(masterSocket, MAX_LISTEN) < 0){
        perror("listen");
        return EXIT_FAILURE;
    }
    //get the status.json file
    char* statusJson;
    {
        FILE* statusFile = fopen("status.json", "r");
        if(statusFile == NULL){
            perror("fopen");
            return EXIT_FAILURE;
        }
        fseek(statusFile, 0, SEEK_END);
        long statusSize = ftell(statusFile);
        rewind(statusFile);
        statusJson = (char*)calloc(statusSize + 1, sizeof(char));
        fread(statusJson, 1, statusSize, statusFile);
        fclose(statusFile);
    }
    nbt_node* codec;
    {
        FILE* codecFile = fopen(templateRegistry, "rb");
        if(codecFile == NULL){
            perror("fopen");
            return EXIT_FAILURE;
        }
        fseek(codecFile, 0, SEEK_END);
        size_t codecSize = ftell(codecFile);
        rewind(codecFile);
        byte* codecData = (byte*)malloc(codecSize);
        fread(codecData, 1, codecSize, codecFile);
        fclose(codecFile);
        codec = nbt_parse(codecData, codecSize);
        if(codec == NULL){
            perror("nbt_parse");
            return EXIT_FAILURE;
        }
    }
    int epollFd = epoll_create1(0);
    if(epollFd < 0){
        perror("epoll_create1");
        return EXIT_FAILURE;
    }
    {//setup log
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("server.log", true));
        auto combined_logger = std::make_shared<spdlog::logger>("serverLogger", begin(sinks), end(sinks));
        //register it if you need to access it globally
        spdlog::register_logger(combined_logger);
        spdlog::set_default_logger(combined_logger);
        spdlog::flush_every(std::chrono::seconds(3));
        spdlog::set_level(spdlog::level::debug);
    }
    server mainServer = server(10, MAX_LOBBIES, MAX_CLIENTS, cJSON_Parse(statusJson), codec, epollFd);
    {
        epoll_event masterEvent;
        masterEvent.events = EPOLLIN;
        masterEvent.data.fd = masterSocket;
        epoll_ctl(epollFd, EPOLL_CTL_ADD, masterSocket, &masterEvent);
    }
    epoll_event events[MAX_CLIENTS];
    spdlog::info("Server started on port {}", PORT);
    while(true){
        //wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        int activity = epoll_wait(epollFd, events, MAX_CLIENTS, infiniteTime);
        if((activity < 0) && (errno != EINTR)){
            perror("select");
            return EXIT_FAILURE;
        }
        for(int i = 0; i < activity; i++){
            if(events[i].events & EPOLLRDHUP){
                client* c = (client*)events[i].data.ptr;
                mainServer.removeClient(c->getIndex());
                delete c;
                continue;
            }
            if(events[i].data.fd == masterSocket){
                int newSocket;
                if((newSocket = accept(masterSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0){
                    perror("accept");
                    return EXIT_FAILURE;
                }
                //set newSocket to non blocking
                if(fcntl(newSocket, F_SETFL, fcntl(newSocket, F_GETFL, 0) | O_NONBLOCK) < 0){
                    perror("fcntl");
                    return EXIT_FAILURE;
                }
                mainServer.createClient(newSocket);
            }
            else if(events[i].data.ptr != NULL){
                client* c = (client*)events[i].data.ptr;
                packet p = c->getPacket();
                int res = 1;
                while(!packetNull(p)){
                    res = c->handlePacket(&p);
                    if(res < 1 || c->getState() == PLAY_STATE){
                        free(p.data);
                        break;
                    }
                    free(p.data);
                    p = c->getPacket();
                }
                if((errno != EAGAIN && errno != EWOULDBLOCK) || c->getState() == PLAY_STATE){
                    spdlog::debug("Server losing track of client {}({})", c->getUUID(), c->getIndex());
                    mainServer.removeClient(c->getIndex());
                    delete c;
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
