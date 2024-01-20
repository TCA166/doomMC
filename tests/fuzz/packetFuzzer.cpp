#include "serverFuzzer.hpp"
#include "../../src/C/networkingMc.h"
#include <spdlog/spdlog.h>

byteArray serverFuzzer::generateFuzzData(){
    byte* data = (byte*)malloc((FUZZER_MAX_SIZE * sizeof(int32_t)) + MAX_VAR_INT);
    byte packetIdBuff[MAX_VAR_INT];
    int32_t packetId = rand() % MAX_PACKET_ID;
    spdlog::debug("Packet ID: {}", packetId);
    size_t packetIdSz = writeVarInt(packetIdBuff, packetId);
    size_t offset = writeVarInt(data, FUZZER_MAX_SIZE * sizeof(int32_t) + packetIdSz);
    memcpy(data + offset, packetIdBuff, packetIdSz);
    offset += packetIdSz;
    int32_t* packetData = (int32_t*)(data + offset);
    //generate random data
    for(int i = 0; i < FUZZER_MAX_SIZE; i++){
        packetData[i] = rand();
    }
    byteArray ret = {(byte*)data, FUZZER_MAX_SIZE * sizeof(int32_t)};
    return ret;
}

int main(int argc, char *argv[]){
    srand(time(NULL));
    spdlog::set_level(spdlog::level::debug);
    serverFuzzer* fuzzer = new serverFuzzer("lastFuzz.bin");
    fuzzer->main();
    return 0;
}