#include "../../src/server.hpp"

extern "C"{
    #include <sys/mman.h>
    #include <unistd.h>
}

int fuzzServer(const uint8_t *data, size_t size){
    server* serv = new server(8080, 10, 0, 1, "status.json", "registryCodec.nbt", "version.json");
    int dummy = memfd_create("dummy", 0);
    serv->createClient(dummy);
    write(dummy, data, size);
    serv->run();
    delete serv;
    return 0;
}

//TODO actually mount the fuzztest
