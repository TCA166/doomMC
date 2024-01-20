#include "../../src/server.hpp"

extern "C"{
    #include <pthread.h>
}

#define FUZZER_MAX_SIZE 64

static void* startServer(void* serv);

class serverFuzzer{
    private:
        server* serv;
        pthread_t thread;
        int pipefd[2];
        FILE* fuzzData;
        byteArray generateFuzzData();
    public:
        serverFuzzer();
        serverFuzzer(const char* fuzzDataLog);
        ~serverFuzzer();
        void fuzz();
        void main();
        void main(int iterCount);
        
};
