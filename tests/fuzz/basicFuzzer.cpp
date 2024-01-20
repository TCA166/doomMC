#include "serverFuzzer.hpp"
#include <spdlog/spdlog.h>

byteArray serverFuzzer::generateFuzzData(){
    int32_t* data = (int32_t*)malloc(FUZZER_MAX_SIZE * sizeof(int32_t));
    //generate random data
    for(int i = 0; i < FUZZER_MAX_SIZE; i++){
        data[i] = rand();
    }
    byteArray ret = {(byte*)data, FUZZER_MAX_SIZE * sizeof(int32_t)};
    return ret;
}

int main(int argc, char** argv){
    int logLevel = spdlog::level::debug;
    int iterCount = 100;
    if(argc > 1){
        logLevel = atoi(argv[1]);
        if(argc > 2){
            iterCount = atoi(argv[2]);
        }
    }
    spdlog::set_level((spdlog::level::level_enum)logLevel);
    srand(time(NULL));
    serverFuzzer* fuzzer = new serverFuzzer("lastFuzz.bin");
    fuzzer->main(iterCount);
    return 0;
}