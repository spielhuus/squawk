#include "scanner.h"

#include <iostream>
#include <vector>

#include "spdlog/spdlog.h"

#include <boost/filesystem.hpp>

static const std::string EVENT_RESCAN = "cds::rescan";
static const std::string LOGGER = "cds";
static const std::string LUA_FLUSH =
    "local keys = redis.call('keys', ARGV[1]) \n for i=1,#keys,5000 do \n redis.call('del', unpack(keys, i, math.min(i+4999, #keys))) \n end \n return keys";

void CdsScanner::rescan ( bool flush, std::function<void ( std::error_code& ) > fn ) {
    std::error_code _errc;

    try {

        //flush the database when requested
        data::eval ( redox_, LUA_FLUSH, 0, "fs:*" );

        //create the content nodes
        //TODO check if root is present, database is created
        save_folder ( param::ROOT, param::ROOT, "" );

        for ( auto& __mod : _internal::menu ) {
            save_folder ( __mod.at ( key::TYPE ), __mod.at ( param::NAME ), param::ROOT );
            data::add_types ( redox_, param::ROOT, __mod.at ( key::TYPE ), data::time_millis() );
        }

        //start import
        redox::Command< std::vector< std::string > >& _c =
            redox_->commandSync< std::vector< std::string > > ( {"LRANGE" "config:directories" "0" "-1" } );

        if ( _c.ok() ) {
            for ( const std::string& __c : _c.reply() ) {
                parse ( __c );
            }
        }

    } catch ( ... ) {
        spdlog::get ( LOGGER )->error ( "exception in rescan." );
        _errc = std::error_code ( 1, std::generic_category() );
    }

    fn ( _errc );
}

TreeItem::TreeItem ( TYPE type, const std::string& path, size_t size, time_t timestamp ) :
    type ( type ), size ( size ), timestamp ( timestamp ), path ( path ) {
}

TreeItem::~TreeItem() {
    std::cout << "desctroy TreeItem" << std::endl;
}

CdsScanner::CdsScanner ( const std::string& redis, const std::string& redis_port )  {

}

void CdsScanner::init ( const std::string& redis, const std::string& redis_port ) {
    redox_ = data::make_connection ( redis, std::stoi ( redis_port ) );

    _magic = magic_open ( MAGIC_MIME_TYPE );
    magic_load ( _magic, nullptr );

    if ( !sub_.connect ( redis, std::stoi ( redis_port ) ) ) {
        spdlog::get ( LOGGER )->warn ( "can not subscribe to redis queue" );

    } else {
        sub_.subscribe ( EVENT_RESCAN, [this] ( const std::string & topic, const std::string & msg ) {
            spdlog::get ( LOGGER )->info ( "COMMAND:{}:{}", topic, msg );

            if ( !rescanning_ ) {
                rescanning_ = true;
                scanner_thread_ = std::make_unique< std::thread > ( &CdsScanner::rescan, this, msg == "true",
                [this] ( std::error_code& errc ) {
                    spdlog::get ( LOGGER )->info ( "scanner fisnished: {}", errc.message() );
                    //TODO send event to redis
                    rescanning_ = false;
                } );
                scanner_thread_->detach();

            } else {
                spdlog::get ( LOGGER )->info ( "scanner already running." );
                //TODO send event to redis
            }
        } );
    }

}
const std::vector< std::regex > CdsScanner::_disc_patterns {
    {   std::regex ( "^(/.*)/CD[0-9]+$", std::regex_constants::icase ),
        std::regex ( "^(/.*)/DISC *[0-9]+$", std::regex_constants::icase ),
        std::regex ( "^(/.*)/DISK[0-9]+$", std::regex_constants::icase )
    }
};

const std::vector< std::regex > CdsScanner::_artwork_patterns {
    {   std::regex ( "^(/.*)/ARTWORK$", std::regex_constants::icase ),
        std::regex ( "^(/.*)/ART$", std::regex_constants::icase ),
        std::regex ( "^(/.*)/COVERS$", std::regex_constants::icase ),
        std::regex ( "^(/.*)/SCANS$", std::regex_constants::icase )
    }
};

const bool CdsScanner::match ( disc_matches_t m, const std::string& path ) {
    for ( auto& _regex : m ) {
        std::smatch matches;

        if ( std::regex_search ( path, matches, _regex ) ) {
            return true;
        }
    }

    return false;
}

const char* CdsScanner::_mime_type_audio = "audio/";
const char* CdsScanner::_mime_type_video = "video/";
const char* CdsScanner::_mime_type_image = "image/";
const char* CdsScanner::_mime_type_pdf = "application/pdf";

void CdsScanner::parse ( std::string& path ) {
    spdlog::get ( LOGGER )->debug ( "import directory %s", path );
}

void CdsScanner::parse ( TreeItem* parent, const std::string& path ) {

    //get files in folder
    boost::filesystem::path _fs_path ( path );
    boost::filesystem::directory_iterator end_itr;

    for ( boost::filesystem::directory_iterator itr ( _fs_path ); itr != end_itr; ++itr ) {
        const std::string _item_filepath = path + "/" + itr->path().filename().string();
        const char* _extension = itr->path().extension().c_str();

        if ( boost::filesystem::is_regular_file ( itr->status() ) ) {

            const char* _mime_type = magic_file ( _magic, itr->path().c_str() );

            TYPE _type = TYPE::REG_FILE;

            if ( strncmp ( _mime_type, _mime_type_audio, strlen ( _mime_type_audio ) ) == 0 ) {
                _type = TYPE::AUDIO;
                ++parent->contents[TYPE::AUDIO];

            } else if ( strncmp ( _mime_type, _mime_type_video, strlen ( _mime_type_video ) ) == 0 ) {
                _type = TYPE::VIDEO;
                ++parent->contents[TYPE::VIDEO];

            } else if ( strncmp ( _mime_type, _mime_type_image, strlen ( _mime_type_image ) ) == 0 ) {
                _type = TYPE::IMAGE;
                ++parent->contents[TYPE::IMAGE];

            } else if ( strncmp ( _mime_type, _mime_type_pdf, strlen ( _mime_type_pdf ) ) == 0 ) {
                _type = TYPE::EBOOK;
                ++parent->contents[TYPE::EBOOK];

            } else if ( strcmp ( _extension, ".cue" ) == 0 ) {
                _type = TYPE::CUE_SHEET;
                ++parent->contents[TYPE::CUE_SHEET];
            }

            parent->children.push_back (
                std::make_shared< TreeItem > ( _type, _item_filepath, boost::filesystem::file_size ( itr->path() ), boost::filesystem::last_write_time ( itr->path() ) ) );

        } else if ( boost::filesystem::is_directory ( itr->status() ) ) {

            TYPE type = TYPE::DIRECTORY;

            if ( match ( _disc_patterns, itr->path().c_str() ) ) {
                type = TYPE::DISC;
                ++parent->contents[TYPE::DISC];

            } else if ( match ( _artwork_patterns, itr->path().c_str() ) ) {
                ++parent->contents[TYPE::ARTWORK];
                type = TYPE::ARTWORK;
            }

            auto node = std::make_shared< TreeItem > ( type, _item_filepath, 0U, boost::filesystem::last_write_time ( itr->path() ) );
            parse ( node.get(), _item_filepath );
            parent->children.push_back ( node );
        }
    }
}

std::shared_ptr< TreeItem > CdsScanner::parse ( const std::string& path ) {
    std::shared_ptr< TreeItem > _node = std::shared_ptr< TreeItem > (
    new TreeItem ( TYPE::DIRECTORY, path, 0U, 0U ), [] ( TreeItem* item ) {
        std::cout << "destroy TreeItem ptr" << std::endl;
    } );
    parse ( _node.get(), path );
    return std::move ( _node );
}

FOLDER_TYPE CdsScanner::type ( TreeItem& item ) {
    if ( item.contents.find ( TYPE::AUDIO ) != item.contents.end() && item.contents.at ( TYPE::AUDIO ) == 1 &&
            item.contents.find ( TYPE::CUE_SHEET ) != item.contents.end() && item.contents.at ( TYPE::CUE_SHEET ) > 0 ) {
        return FOLDER_TYPE::AUDIO_IMAGE_CUE;

    } else if ( item.contents.find ( TYPE::AUDIO ) != item.contents.end() && item.contents.at ( TYPE::AUDIO ) > 1 &&
                item.contents.find ( TYPE::CUE_SHEET ) != item.contents.end() && item.contents.at ( TYPE::CUE_SHEET ) > 0 ) {
        return FOLDER_TYPE::AUDIO_CUE;

    } else if ( item.contents.find ( TYPE::AUDIO ) != item.contents.end() && item.contents.at ( TYPE::AUDIO ) > 0 ) {
        return FOLDER_TYPE::AUDIO;

    } else if ( item.contents.find ( TYPE::DISC ) != item.contents.end() && item.contents.at ( TYPE::DISC ) > 0 ) {
        return FOLDER_TYPE::AUDIO_MULTIDISC;

    } else if ( item.contents.find ( TYPE::EBOOK ) != item.contents.end() && item.contents.at ( TYPE::EBOOK ) > 0 ) {
        return FOLDER_TYPE::EBOOK;

    } else { std::cout << ">> UNKNOWN: " << item.path << ", " << item.contents << "\n"; }

    return FOLDER_TYPE::UNKNOWN;
}

void visit ( TreeItem& item, std::function< void ( TreeItem& ) > fn ) {

}

void CdsScanner::save_folder ( const std::string& key /** @param path path of the node. */,
                            const std::string& name /** @param name name of the node. */,
                            const std::string& parent /** @param parent parent key. */ ) {
    data::node_t _node {
        { param::NAME, name },
        { param::PARENT, parent },
        { param::CLASS, data::NodeType::str ( data::NodeType::folder ) },
        { param::TIMESTAMP, std::to_string ( data::time_millis() ) },
    };
    data::save ( redox_, key, _node );
}
