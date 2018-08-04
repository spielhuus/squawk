#include "scanner2.h"

#include "spdlog/spdlog.h"

Scanner2::Scanner2 ( const std::string& redis, const std::string& redis_port )
    : redox_ ( data::make_connection ( redis, std::stoi ( redis_port )  ) ) {

}

