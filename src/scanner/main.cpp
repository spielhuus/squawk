#include <iostream>

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

#include "cdsscanner.h"

static const char* PARAM_REDIS              = "redis";
static const char* PARAM_REDIS_PORT         = "redis-port";

[[ noreturn ]] void signalHandler ( int signum ) {
    std::cout << "CDS:API:Interrupt signal (" << signum << ") received.\n" << std::endl;
    exit ( signum );
}

int main ( int argc, char* argv[] ) {

    cxxopts::Options options ( argv[0], "Squawk content directory server." );
    options.add_options()
    ( PARAM_REDIS, "Redis Database (default: localhost)", cxxopts::value<std::string>()->default_value ( "localhost" ), "HOST" )
    ( PARAM_REDIS_PORT, "Redis Database port (default: 6379)", cxxopts::value<std::string>()->default_value ( "6379" ), "PORT" )
    ( "help", "Print help" );

    auto result = options.parse ( argc, argv );

    if ( result.count ( "help" ) ) {
        std::cout << options.help ( {"", "Group"} ) << std::endl;
        exit ( 0 );
    }

    const auto& _redis_server = result[PARAM_REDIS].as<std::string>();
    const auto& _redis_port = result[PARAM_REDIS_PORT].as<std::string>();

//    auto redox_ = data::make_connection ( _redis_server, std::stoi ( _redis_port ), REDIS_DATABASE::CONFIG );

//    /* configure server */
//    std::vector< Parameters > _parameter {{
//            {PARAM_LISTEN_ADDRESS, config::UPNP_LISTEN_ADDRESS, "localhost"},//TODO get ip
//            {PARAM_LISTEN_PORT, config::UPNP_LISTEN_PORT, "9012"}
//        }
//    };

//    server_config ( redox_, result, _parameter );

    /* setup logger */
    spdlog::set_level ( spdlog::level::trace );
    auto console = spdlog::stdout_color_mt ( SCANNER_LOGGER );

    console->info ( "Start Scanner (Redis={}:{})", _redis_server, _redis_port );

    auto _scanner = std::make_shared< scanner::CdsScanner > ( _redis_server, _redis_port );

    // register signal SIGINT and signal handler
    signal ( SIGINT, signalHandler );

    while ( 1 )
    { sleep ( 1 ); }
}
