#include <iostream>

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

#include "cmsserver.h"

static const std::string LOGGER = "cms";

static const char* PARAM_REDIS              = "redis";
static const char* PARAM_REDIS_PORT         = "redis-port";
static const char* PARAM_LISTEN_ADDRESS     = "listen";
static const char* PARAM_HTTP_PORT          = "http-port";

[[ noreturn ]] void signalHandler ( int signum ) {
    std::cout << "CMS:Interrupt signal (" << signum << ") received.\n" << std::endl;
    exit ( signum );
}

int main ( int argc, char* argv[] ) {

    cxxopts::Options options ( argv[0], "Squawk content manager server." );
    options.add_options()
    ( PARAM_LISTEN_ADDRESS, "CMS Webserver IP-Adress to bind to.", cxxopts::value<std::string>()->default_value ( "0.0.0.0" ), "IP" )
    ( PARAM_HTTP_PORT, "CMS Webserver IP-Port to bind to.", cxxopts::value<std::string>()->default_value ( "9002" ), "PORT" )
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

    /* configure server */

    auto redox_ = data::make_connection ( _redis_server, std::stoi ( _redis_port ) );

    if ( result.count ( PARAM_LISTEN_ADDRESS ) ) {
        const auto& _listen_address = result[PARAM_LISTEN_ADDRESS].as<std::string>();
        redox_->set ( "config:cms:listen_address", _listen_address );

    } else if ( !data::exists ( redox_, "config:cms:listen_address" ) ) {
        redox_->set ( "config:cms:listen_address", "localhost" );
    }

    if ( result.count ( PARAM_HTTP_PORT ) ) {
        const auto& _listen_port = result[PARAM_HTTP_PORT].as<std::string>();
        redox_->set ( "config:cms:listen_port", _listen_port );

    } else if ( !data::exists ( redox_, "config:cms:listen_port" ) ) {
        redox_->set ( "config:cms:listen_address", "9002" );
    }

    /* setup logger */
    spdlog::set_level ( spdlog::level::trace );
    auto console = spdlog::stdout_color_mt ( LOGGER );

    console->info ( "Start CMS server ({}:{})", _listen_address, _http_port );

    auto _api_server = std::make_shared< CmsServer > (
                           _redis_server, _redis_port, _listen_address, _http_port );

    // register signal SIGINT and signal handler
    signal ( SIGINT, signalHandler );

    while ( 1 )
    { sleep ( 1 ); }
}
