#ifndef SQUAWK_UTILS_H
#define SQUAWK_UTILS_H

#include "cxxopts.hpp"

#include <string>
#include <vector>

#include "datastore.h"

// handle the server configuration

struct Parameters {
    std::string flag;
    std::string key;
    std::string value;
};

static void server_config ( data::redis_ptr redis /** @param redis the database pointer. */,
                            cxxopts::ParseResult& result, std::vector< Parameters >& _parameter ) {
    for ( auto& __parameter : _parameter ) {
        if ( result.count ( __parameter.flag ) ) {
            const auto& _value = result[__parameter.flag].as<std::string>();
            redis->set ( __parameter.key, _value );
            redis->publish ( __parameter.key, _value );

        } else if ( !data::exists ( redis, __parameter.key ) ) {
            redis->set ( __parameter.key, __parameter.value );
            redis->publish ( __parameter.key, __parameter.value );
        }
    }
}

inline std::string filename ( const std::string& filename, bool extension = true ) {
    std::string _filename;

    if ( filename.find ( "/" ) != std::string::npos ) {
        _filename = filename.substr ( filename.rfind ( "/" ) + 1 );

    } else {
        _filename = filename;
    }

    if ( !extension ) {
        if ( _filename.find ( "." ) != std::string::npos ) {
            _filename.erase ( _filename.rfind ( "." ) );
        }
    }

    return _filename;
}
#endif // USQUAWK_TILS_H
