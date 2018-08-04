#ifndef CMSSERVER_H
#define CMSSERVER_H

#include <memory>
#include <mutex>
#include <string>

#include "http/server.h"
#include "http/httpserver.h"

#include "../common/datastore.h"

class CmsServer {
public:
    CmsServer ( const std::string& redis, const std::string& redis_port,
                const std::string& listen_address, const std::string& protocol );

private:
    const data::redis_ptr redox_;
    const std::shared_ptr< http::Server< http::HttpServer > > http_server_;
};

#endif // CMSSERVER_H
