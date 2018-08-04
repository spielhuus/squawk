#include <iostream>

#include <memory>
#include <mutex>
#include <string>

#include "http/server.h"
#include "http/httpserver.h"

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

static const std::string LOGGER = "cds::api";

static const char* PARAM_REDIS              = "redis";
static const char* PARAM_REDIS_PORT         = "redis-port";
static const char* PARAM_LISTEN_ADDRESS     = "listen";
static const char* PARAM_HTTP_PORT          = "http-port";

[[ noreturn ]] void signalHandler ( int signum ) {
    std::cout << "CDS:API:Interrupt signal (" << signum << ") received.\n" << std::endl;
    exit ( signum );
}

int main ( int argc, char* argv[] ) {

    cxxopts::Options options ( argv[0], "Squawk content directory server." );
    options.add_options()
    ( PARAM_LISTEN_ADDRESS, "API Webserver IP-Adress to bind to.", cxxopts::value<std::string>()->default_value ( "0.0.0.0" ), "IP" )
    ( PARAM_HTTP_PORT, "API Webserver IP Port to bind to.", cxxopts::value<std::string>()->default_value ( "9001" ), "PORT" )
    ( PARAM_REDIS, "Redis Database (default: localhost)", cxxopts::value<std::string>()->default_value ( "localhost" ), "HOST" )
    ( PARAM_REDIS_PORT, "Redis Database port (default: 6379)", cxxopts::value<std::string>()->default_value ( "6379" ), "PORT" )
    ( "help", "Print help" )
    ;

    auto result = options.parse ( argc, argv );

    if ( result.count ( "help" ) ) {
        std::cout << options.help ( {"", "Group"} ) << std::endl;
        exit ( 0 );
    }

    const auto& _redis_server = result[PARAM_REDIS].as<std::string>();
    const auto& _redis_port = result[PARAM_REDIS_PORT].as<std::string>();
    const auto& _listen_address =  result[PARAM_LISTEN_ADDRESS].as<std::string>();
    const auto& _http_port = result[PARAM_HTTP_PORT].as<std::string>();

    /* setup logger */
    spdlog::set_level ( spdlog::level::trace );
    auto console = spdlog::stdout_color_mt ( LOGGER );

    console->info ( "Start WWW server ({}:{})", _listen_address, _http_port );

    //TODO Start server

    // register signal SIGINT and signal handler
    signal ( SIGINT, signalHandler );

    while ( 1 )
    { sleep ( 1 ); }
}
