#include "server.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#define MAX_LOBBIES 1
#define MAX_CLIENTS 100
#define PORT 8080

int main(int argc, char *argv[]){
    int logLevel = spdlog::level::debug;
    if(argc > 1){
        logLevel = atoi(argv[1]);
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
        spdlog::set_level((spdlog::level::level_enum)logLevel);
    }
    try{
        server mainServer = server(PORT, 10, MAX_LOBBIES, MAX_CLIENTS, "status.json", "registryCodec.nbt", "version.json");
        spdlog::info("Server starting on port {}", PORT);
        return mainServer.run();
    }
    catch(const std::exception& e){
        spdlog::error("{}", e.what());
        return 1;
    }
    catch(const std::error_code& e){
        spdlog::error("{} exception. Code:{}, {}", e.category().name(), e.value(), e.message());
        return 1;
    }
    catch(...){
        spdlog::error("Unknown error");
        return 1;
    }
}