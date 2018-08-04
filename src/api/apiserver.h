#ifndef APISERVER_H
#define APISERVER_H

#include <memory>
#include <mutex>
#include <string>

#include "http/server.h"
#include "http/httpserver.h"

#include "../common/datastore.h"

inline bool request_html ( http::Request& request ) {
    if ( request.contains_attribute ( "json" ) && request.attribute ( "json" ) == "true" )
    {return false;}

    return request.parameter ( "Accept" ).find ( "text/html" ) != std::string::npos;
}

class ApiServer {
public:
    ApiServer ( const std::string& redis, const std::string& redis_port );

    /** @brief save configuration. */
    http::http_status save_config ( http::Request& request, http::Response& response );
    /** @brief get configuration. */
    http::http_status config ( http::Request& request, http::Response& response );

    /** @brief rescan media directories. */
    http::http_status rescan ( http::Request& request, http::Response& response );
    /** @brief get content directory status. */
    http::http_status status ( http::Request& request, http::Response& response );

    /** @brief get node item. */
    http::http_status node ( http::Request& request, http::Response& response );
    /** @brief get node list. */
    http::http_status nodes ( http::Request& request, http::Response& response );

    /** @brief get node item. */
    http::http_status query ( http::Request& request, http::Response& response );

private:
    const data::redis_ptr redox_;
    const data::redis_ptr redox_config_;
    const std::shared_ptr< http::Server< http::HttpServer > > http_server_;
    std::atomic<bool> rescanning_;
};

#endif // APISERVER_H
