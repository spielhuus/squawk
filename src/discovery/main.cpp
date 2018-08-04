#include <iostream>

#include <signal.h>

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

#include "ssdpserver.h"

#include "../common/constants.h"
#include "../common/utils.h"

static const char* PARAM_REDIS              = "redis";
static const char* PARAM_REDIS_PORT         = "redis-port";
static const char* PARAM_LISTEN_ADDRESS     = "listen";
static const char* PARAM_LISTEN_PORT        = "port";
static const char* PARAM_MULTICAST_ADDRESS  = "multicast";
static const char* PARAM_MULTICAST_PORT     = "multicast-port";
static const char* PARAM_NAME               = "name";
static const char* PARAM_UUID               = "uuid";

[[ noreturn ]] void signalHandler ( int signum ) {
    std::cout << "CDS:DISCOVERY:Interrupt signal (" << signum << ") received.\n" << std::endl;
    exit ( signum );
}

int main ( int argc, char* argv[] ) {

    cxxopts::Options options ( argv[0], "Squawk content directory server." );
    options.add_options()
    ( PARAM_REDIS, "Redis Database (default: localhost)", cxxopts::value<std::string>()->default_value ( "localhost" ), "HOST" )
    ( PARAM_REDIS_PORT, "Redis Database port (default: 6379)", cxxopts::value<std::string>()->default_value ( "6379" ), "PORT" )
    ( PARAM_LISTEN_ADDRESS, "Listen for SSDP multicast messages in the network.", cxxopts::value<std::string>(), "IP" )
    ( PARAM_LISTEN_PORT, "Listen for HTTP PORT to bind.", cxxopts::value<std::string>(), "PORT" )
    ( PARAM_MULTICAST_ADDRESS, "SSDP multicast IP-Adress to bind to.", cxxopts::value<std::string>(), "IP" )
    ( PARAM_MULTICAST_PORT, "SSDP multicast port to bind to.", cxxopts::value<std::string>(), "PORT" )
    ( PARAM_NAME, "Server display name (default: empty)).", cxxopts::value<std::string>(), "NAME" )
    ( "help", "Print help" );

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
            {PARAM_NAME, config::NAME, "Squawk Media Server (Test)"},//TODO get version
            {PARAM_UUID, config::UUID, "7799a4b3-9c03-49a2-1bce-aebbf4512dc8"}, //TODO generate uuid
            {PARAM_LISTEN_ADDRESS, config::UPNP_LISTEN_ADDRESS, "localhost"},//TODO get ip
            {PARAM_LISTEN_PORT, config::UPNP_LISTEN_PORT, "9012"},
            {PARAM_MULTICAST_ADDRESS, config::UPNP_MULTICAST_ADDRESS, "239.255.255.250"},
            {PARAM_MULTICAST_PORT, config::UPNP_MULTICAST_PORT, "1900"}
        }
    };

    server_config ( redox_, result, _parameter );

    /* setup logger */
    spdlog::set_level ( spdlog::level::trace );
    auto console = spdlog::stdout_color_mt ( DISCOVERY_LOGGER );

    console->info ( "Start SSDP Server (Redis={}:{})", _redis_server, _redis_port );

    //create ssdp server
    auto ssdp = std::make_shared< upnp::SSDPServerImpl > ( _redis_server, _redis_port );
    ssdp->announce();

    // register signal SIGINT and signal handler
    signal ( SIGINT, signalHandler );

    while ( 1 )
    { sleep ( 1 ); }
}
