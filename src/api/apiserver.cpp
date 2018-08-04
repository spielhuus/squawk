#include "apiserver.h"

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

#include <nlohmann/json.hpp>
#include <inja/inja.hpp>

#include "../common/constants.h"
#include "../common/datastore.h"
#include "../common/utils.h"

#include "config.html.h"
#include "index.html.h"
#include "main.css.h"

using namespace inja;
using json = nlohmann::json;

using namespace std::placeholders;

static const std::string EVENT_RESCAN = "cds::rescan";
static const std::string VERSION = "0.0.0";

ApiServer::ApiServer ( const std::string& redis, const std::string& redis_port )
    : redox_ ( data::make_connection ( redis, std::stoi ( redis_port ), REDIS_DATABASE::FS ) ),
      redox_config_ ( data::make_connection ( redis, std::stoi ( redis_port ), REDIS_DATABASE::CONFIG ) ),
      http_server_ ( std::shared_ptr< http::Server< http::HttpServer > > (
                         new http::Server< http::HttpServer > ( redox_config_->get ( config::API_LISTEN_ADDRESS ), redox_config_->get ( config::API_LISTEN_PORT ) ) ) ) {

    http_server_->bind ( http::mod::Match<> ( "^\\/(index.html)?$" ),
    http::mod::Exec ( [] ( http::Request&, http::Response& response ) {
        response << HTML_INDEX;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::HTML ) );
        response.parameter ( "Access-Control-Allow-Origin", "*" );
        return http::http_status::OK;
    } ),
    http::mod::Http() );

    http_server_->bind ( http::mod::Match<> ( "^\\/config$" ),
                         http::mod::Method (
                             http::mod::method::POST ( http::mod::Exec ( std::bind ( &ApiServer::save_config, this, _1, _2 ) ) ),
                             http::mod::method::GET ( http::mod::Exec ( std::bind ( &ApiServer::config, this, _1, _2 ) ) ) ),
                         http::mod::Http() );

    http_server_->bind ( http::mod::Match<> ( "^\\/rescan$" ),
                         http::mod::Exec ( std::bind ( &ApiServer::rescan, this, _1, _2 ) ),
                         http::mod::Http() );

    http_server_->bind ( http::mod::Match<> ( "^\\/status$" ),
                         http::mod::Exec ( std::bind ( &ApiServer::status, this, _1, _2 ) ),
                         http::mod::Http() );

    http_server_->bind ( http::mod::Match< std::string > ( "^\\/+((root)|[[:digit:]]+)$", key::KEY ),
                         http::mod::Exec ( std::bind ( &ApiServer::node, this, _1, _2 ) ),
                         http::mod::Http()
                       );

    http_server_->bind ( http::mod::Match< std::string > ( "^\\/+((file|ebook|movie|album|serie|artist|image)|[[:digit:]]+)$", key::KEY ),
                         http::mod::Exec ( std::bind ( &ApiServer::query, this, _1, _2 ) ),
                         http::mod::Http()
                       );

    http_server_->bind ( http::mod::Match<std::string> ( "^\\/+(.+)\\/+nodes$", key::KEY ),
                         http::mod::Exec ( std::bind ( &ApiServer::nodes, this, _1, _2 ) ),
                         http::mod::Http()
                       );

    http_server_->bind ( http::mod::Match<> ( "^\\/main.css$" ),
    http::mod::Exec ( [] ( http::Request&, http::Response& response ) {
        response << CSS_CONFIG;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::CSS ) );
        response.parameter ( "Access-Control-Allow-Origin", "*" );
        return http::http_status::OK;
    } ),
    http::mod::Http() );
}

http::http_status ApiServer::save_config ( http::Request& request, http::Response& response ) {
    std::cout << request << std::endl;
    return config ( request, response );
}

http::http_status ApiServer::config ( http::Request& request, http::Response& response ) {

    //get the data
    json data { {"media", json::array() } };

    redox::Command< std::vector< std::string > >& _c =
        redox_config_->commandSync< std::vector< std::string > > ( { "keys", "config:*" } );

    if ( _c.ok() ) {

        for ( const std::string& __c : _c.reply() ) {
            const std::string _type = data::type ( redox_config_, __c );

            if ( _type == "string" ) {
                data[__c] = redox_config_->get ( __c );

            } else {
                redox::Command< std::vector< std::string > >& _list_c =
                    redox_config_->commandSync< std::vector< std::string > > ( { "LRANGE", "config:media", "0", "-1" } );

                json _media = json::array();

                if ( _list_c.ok() ) {

                    for ( const std::string& __c : _list_c.reply() ) {
                        _media.push_back ( __c );
                    }
                }

                data ["media"].swap ( _media );
            }
        }
    }

    if ( request_html ( request ) ) {

        // For more advanced usage, an environment is recommended
        Environment env = Environment();

        // Render a string with json data
        std::string result = env.render ( HTML_CONFIG, data );

        response << result;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::HTML ) );
        response.parameter ( "Access-Control-Allow-Origin", "*" );
        return http::http_status::OK;

    } else {
        response << data;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::JSON ) );
    }

    response.parameter ( "Access-Control-Allow-Origin", "*" );
    return http::http_status::OK;
}

http::http_status ApiServer::rescan ( http::Request& request, http::Response& response ) {
    const std::string _flush = ( request.contains_attribute ( "flush" ) && request.attribute ( "flush" ) == "true" ) ? "true" : "false";
    SPDLOG_DEBUG ( spdlog::get ( API_LOGGER ), "HTTP>/rescan (flush={})", _flush );
//TODO    if ( !rescanning_ ) { //? TODO dont double check
    redox_->publish ( EVENT_RESCAN, _flush );
    response << "{\"code\":200, \"result\":\"OK\"}";

//    } else {
//        response << "{\"code\":400, \"result\":\"ressource busy.\"}";
//    }

    response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::JSON ) );
    response.parameter ( "Access-Control-Allow-Origin", "*" );
    return http::http_status::ACCEPTED;
}

void get_status ( data::redis_ptr redox_, const std::string key, std::function< void ( const std::string& key ) > fn ) {
    data::command_t _command = { redis::ZRANGE, key, "0", "-1" };
    redox::Command< data::nodes_t >& c =
        redox_->commandSync< data::nodes_t > ( _command );

    if ( c.ok() ) {
        for ( const std::string& __mime : c.reply() ) {
            fn ( __mime );

            if ( data::exists ( redox_, data::make_key ( "fs", __mime, "list" ) ) ) {
                get_status ( redox_, data::make_key ( "fs", __mime, "list" ), fn );
            }
        }
    }
}

http::http_status ApiServer::status ( http::Request&, http::Response& response ) {

    //get the data
    json data {
        {"version", "0.0.0"},
        {"rescan", "false"},
        {   "content", {
                {"folder", 0},
                {"audio", 0},
                {"movie", 0},
                {"serie", 0},
                {"image", 0},
                {"ebook", 0},
                {"file", 0},
                {"album", 0},
                {"cover", 0},
                {"episode", 0},
                {"artist", 0},
                {"cue", 0},
                {"logfile", 0},
            }
        }
    };

    data["version"] = VERSION;
    data["rescan"] = ( rescanning_ ? "true" : "false" );

    get_status ( redox_, "fs:root:list", [this,&data] ( const std::string& key ) {
        const std::string _key = data::make_key ( "fs", key );
        const std::string _str_type = data::get ( redox_, _key, "type" );
        const FILE_TYPE _type = static_cast<FILE_TYPE> ( std::stoi ( _str_type ) );

        data["content"][std::to_string ( _type )] =
            ( data["content"][std::to_string ( _type )].get<int>() + 1 );
    } );

    response << data;
    response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::JSON ) );
    response.parameter ( "Access-Control-Allow-Origin", "*" );
    return http::http_status::OK;
}

http::http_status ApiServer::node ( http::Request& request, http::Response& response ) {
    SPDLOG_DEBUG ( spdlog::get ( API_LOGGER ), "HTTP>/node (key={})", request.attribute ( key::KEY ) );
    using namespace rapidjson;
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer ( sb );
    writer.StartObject();
    writer.String ( key::KEY );
    writer.String ( request.attribute ( key::KEY ).c_str() );

    auto& _item = redox_->commandSync<data::command_t> ( {
        redis::HGETALL, data::make_key ( "fs", request.attribute ( key::KEY ) )
    } );

    if ( _item.ok() ) {
        for ( auto& __i : _item.reply() ) {
            writer.String ( __i.c_str() );
        }
    }

    writer.EndObject();
    response << sb.GetString();
    response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::JSON ) );
    response.parameter ( "Access-Control-Allow-Origin", "*" );
    return http::http_status::OK;
}

http::http_status ApiServer::nodes ( http::Request& request, http::Response& response ) {

    std::string _key = request.attribute ( key::KEY );
    int _index = ( request.contains_attribute ( "index" ) &&
                   ( request.attribute ( "index" ).find_first_not_of ( "0123456789" ) == std::string::npos ) ?
                   std::stoi ( request.attribute ( "index" ) ) : 0 );
    int _count = ( request.contains_attribute ( "count" ) &&
                   ( request.attribute ( "count" ).find_first_not_of ( "0123456789" ) == std::string::npos ) &&
                   std::stoi ( request.attribute ( "count" ) ) < 8192 ?
                   std::stoi ( request.attribute ( "count" ) ) : -1 );
    std::string _sort = ( request.contains_attribute ( "sort" ) && !request.attribute ( "sort" ).empty() ?
                          request.attribute ( "sort" ) : "default" );
    std::string _order = ( request.contains_attribute ( "order" ) && ( request.attribute ( "order" ) =="asc" || request.attribute ( "order" ) =="desc" ) ?
                           request.attribute ( "order" ) : "asc" );
    std::string _filter = ( request.contains_attribute ( "filter" ) ? request.attribute ( "filter" ) : "" );

    SPDLOG_DEBUG ( spdlog::get ( API_LOGGER ), "HTTP>/nodes (key={}, index={}, count={}, sort={}, order={}, filter={})", _key, _index, _count, _sort, _order, _filter );

    //get the data
    int _counter = 0;
    json data = json::array();

    data::children ( redox_, _key, _index, _count, _sort, _order, _filter, [this,&_counter,&data] ( const std::string& key ) {

        _counter++;
        json _obj = json::object();;
        _obj[key::KEY] = key.c_str();

        auto n = data::node ( redox_, key );

        for ( auto& __item : n ) {
            _obj[__item.first.c_str()] = __item.second.c_str();
        }

        data.push_back ( _obj );
    } );

    json _json_tree ( {} );
//TODO _json_tree ["count"] = data::children_count ( redox_, _key );
    _json_tree ["result"] = _counter;
    _json_tree.push_back ( { "nodes", data } );

    if ( request_html ( request ) ) {
        // For more advanced usage, an environment is recommended
        Environment env = Environment();

        // Render a string with json data
        std::string result = env.render ( R"inja({% for node in nodes %}
                                          {% if node/cls == "album" %}<p>{{ node/cls }}<a href="/{{ node/key }}/nodes"><img src="https://ia802607.us.archive.org/32/items/mbid-76df3287-6cda-33eb-8e9a-044b5e15ffdd/mbid-76df3287-6cda-33eb-8e9a-044b5e15ffdd-829521842_thumb250.jpg" border="0" alternate-text="{{ node/name }}"/></a></p>{% endif %}
                                          {% else if node/cls == "audio" %}<p>{{ node/mime_type }}<a href="/{{ node/key }}/nodes">{{ node/name }}</a></p>{% endelseif %}
                                          {% else %}<p>{{ node/cls }}<a href="/{{ node/key }}/nodes">{{ node/name }}</a></p>{% endelse %}
                                          {% endfor %})inja", _json_tree );

        response << result;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::HTML ) );

    } else {
        response << _json_tree;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::JSON ) );
    }

    response.parameter ( "Access-Control-Allow-Origin", "*" );
    return http::http_status::OK;
}

http::http_status ApiServer::query ( http::Request& request, http::Response& response ) {
    std::string _key = request.attribute ( key::KEY );

    SPDLOG_DEBUG ( spdlog::get ( API_LOGGER ), "query: {}", _key ) ;

    json data = {};
    json _nodes = json::array();
    data::nodes ( redox_, FILE_TYPE::album, "index:files", [&_nodes] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {

        auto& _item = redis->commandSync< std::vector< std::string > > ( { redis::HGETALL, data::make_key ( "fs", key ) } );

        if ( _item.ok() ) {
            json _obj = {};
            _obj[key::KEY] = key;

            for ( std::size_t i=0; i<_item.reply().size(); ++++i ) {
                if ( i+1 >= _item.reply().size() )
                {SPDLOG_DEBUG ( spdlog::get ( API_LOGGER ), "array overflow." ) ;}

                else
                {_obj[_item.reply().at ( i )] = _item.reply().at ( i+1 );}
            }

            _nodes.push_back ( _obj );
        }
        return std::error_code();
    } );

    data["nodes"] = _nodes;

    //return the content
    if ( request_html ( request ) ) {
        // For more advanced usage, an environment is recommended
        Environment env = Environment();

        // Render a string with json data
        std::string result = env.render ( R"inja({% for node in nodes %}
                                        <p><a href="/{{ node/cls }}/nodes">{{ node/name }}</a></p>
                                        {% endfor %})inja", data );

        response << result;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::HTML ) );

    } else {
        response << data;
        response.parameter ( http::header::CONTENT_TYPE, http::mime::mime_type ( http::mime::JSON ) );
    }

    response.parameter ( "Access-Control-Allow-Origin", "*" );
    return http::http_status::OK;
}
