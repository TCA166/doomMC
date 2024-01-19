#include "../../src/server.hpp"
#include <spdlog/spdlog.h>

extern "C"{
    #include <sys/mman.h>
    #include <unistd.h>
    #include <pthread.h>
}

void* startServer(void* serv){
    spdlog::info("Server starting on port {}", 8080);
    ((server*)serv)->run();
    spdlog::warn("Server stopped");
    return NULL;
}

#define FUZZER_MAX_SIZE 64

#include <random>

int main(int argc, char** argv){
    spdlog::set_level(spdlog::level::debug);
    server* serv = new server(8080, 10, 0, 1, "status.json", "registryCodec.nbt", "version.json");
    int pipefd[2];
    if(pipe(pipefd) != 0){
        spdlog::error("Failed to create pipe");
        return 1;
    }
    serv->createClient(pipefd[0]);
    //start server in thread
    pthread_t thread;
    pthread_create(&thread, NULL, startServer, serv);
    spdlog::info("Starting fuzzing");
    FILE* fuzzData = fopen("lastFuzz.bin", "wb");
    srand(time(NULL));
    // Fuzz server
    while(true){
        uint32_t data[FUZZER_MAX_SIZE];
        //generate random data
        for(int i = 0; i < FUZZER_MAX_SIZE; i++){
            data[i] = rand();
        }
        fseek(fuzzData, 0, SEEK_SET);
        fwrite(data, sizeof(uint32_t), FUZZER_MAX_SIZE, fuzzData);
        // Send data
        if(write(pipefd[1], data, FUZZER_MAX_SIZE * sizeof(uint32_t)) != FUZZER_MAX_SIZE * sizeof(uint32_t)){
            spdlog::error("Failed to write to server");
            return 1;
        }
    }
    return 0;
}
