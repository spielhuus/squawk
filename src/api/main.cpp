#include <iostream>

#include <signal.h>

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

#include "apiserver.h"

#include "../common/constants.h"
#include "../common/utils.h"

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

    auto redox_ = data::make_connection ( _redis_server, std::stoi ( _redis_port ), REDIS_DATABASE::CONFIG );

    /* configure server */
    std::vector< Parameters > _parameter {{
            {PARAM_LISTEN_ADDRESS, config::API_LISTEN_ADDRESS, "localhost"},//TODO get ip
            {PARAM_HTTP_PORT, config::API_LISTEN_PORT, "9010"}
        }
    };

    server_config ( redox_, result, _parameter );

    /* setup logger */
    spdlog::set_level ( spdlog::level::trace );
    auto console = spdlog::stdout_color_mt ( API_LOGGER );

    console->info ( "Start API Server (Redis={}:{})", _redis_server, _redis_port );

    auto _api_server = std::make_shared< ApiServer > ( _redis_server, _redis_port );

    // register signal SIGINT and signal handler
    signal ( SIGINT, signalHandler );

    while ( 1 )
    { sleep ( 1 ); }
}
