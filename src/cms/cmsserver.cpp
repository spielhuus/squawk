#include "cmsserver.h"

#include "spdlog/spdlog.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "http/mod/exec.h"
#include "http/mod/file.h"
#include "http/mod/match.h"
#include "http/mod/method.h"
#include "http/mod/http.h"

using namespace std::placeholders;

static const std::string LOGGER = "cms";

CmsServer::CmsServer ( const std::string& redis, const std::string& redis_port,
                       const std::string& listen_address, const std::string& protocol )
    : redox_ ( data::make_connection ( redis, std::stoi ( redis_port ) ) ),
      http_server_ ( std::shared_ptr< http::Server< http::HttpServer > > (
                         new http::Server< http::HttpServer > ( listen_address, protocol ) ) ) {

//    http_server_->bind ( http::mod::Match<> ( "^\\/config$" ),
//                         http::mod::Exec ( std::bind ( &CmsServer::config, this, _1, _2 ) ),
//                         http::mod::Http() );

//    http_server_->bind ( http::mod::Match<> ( "^\\/rescan$" ),
//                         http::mod::Exec ( std::bind ( &CmsServer::rescan, this, _1, _2 ) ),
//                         http::mod::Http() );

//    http_server_->bind ( http::mod::Match<> ( "^\\/status$" ),
//                         http::mod::Exec ( std::bind ( &CmsServer::status, this, _1, _2 ) ),
//                         http::mod::Http() );

//    http_server_->bind ( http::mod::Match< std::string > ( "^\\/+((root|file|ebook|movie|album|serie|artist|image)|[[:digit:]]+)$", key::KEY ),
//                         http::mod::Exec ( std::bind ( &CmsServer::node, this, _1, _2 ) ),
//                         http::mod::Http()
//                       );

    http_server_->bind ( http::mod::Match<> ( "^\\/main.css$" ),
    http::mod::Exec ( [] ( http::Request& request, http::Response& response ) {
        std::cout << "REQUETS:" << request<< std::endl;
//        response << CSS_CONFIG;
//        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::CSS ) );
//        response.parameter ( "Access-Control-Allow-Origin", "*" );
        return http::http_status::OK;
    } ),
    http::mod::Http() );
}
