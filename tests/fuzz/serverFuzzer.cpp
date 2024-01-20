#include "serverFuzzer.hpp"
#include <spdlog/spdlog.h>

extern "C"{
    #include <unistd.h>
}

static void* startServer(void* serv){
    spdlog::info("Server starting on port {}", 8080);
    ((server*)serv)->run();
    spdlog::warn("Server stopped");
    return NULL;
}

serverFuzzer::serverFuzzer(const char* fuzzDataLog){
    serv = new server(8080, 10, 0, 1, "status.json", "registryCodec.nbt", "version.json");
    if(pipe(pipefd) != 0){
        throw std::error_code(errno, std::generic_category());
    }
    serv->createClient(pipefd[0]);
    //start server in thread
    if(pthread_create(&thread, NULL, startServer, serv) < 0){
        throw std::error_code(errno, std::generic_category());
    }
    this->fuzzData = fopen(fuzzDataLog, "wb");
}

serverFuzzer::serverFuzzer(){
    serverFuzzer("fuzzData.bin");
}

serverFuzzer::~serverFuzzer(){
    fclose(this->fuzzData);
    close(pipefd[0]);
    close(pipefd[1]);
    pthread_cancel(thread);
    delete serv;
}

void serverFuzzer::fuzz(){
    byteArray data = this->generateFuzzData();
    fseek(this->fuzzData, 0, SEEK_SET);
    fwrite(data.bytes, 1, data.len, fuzzData);
    // Send data
    spdlog::debug("Sending {} bytes to server", data.len);
    client* c = serv->getClient(0);
    if(c == NULL){
        throw std::runtime_error("Client not found; we probably got kicked");
    }
    if(write(pipefd[1], data.bytes, data.len) != data.len){
        spdlog::error("Failed to write to server");
        throw std::error_code(errno, std::generic_category());
    }
    free(data.bytes);
}

void serverFuzzer::main() {
    main(100);
}

void serverFuzzer::main(int iterCount){
    int i = 0;
    for(; i < iterCount; i++){
        try{
            this->fuzz();
        }
        catch(const std::exception& e){
            spdlog::error("{}", e.what());
            break;
        }
    }
    spdlog::info("Fuzzing {} times complete", i);
}
