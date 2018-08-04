/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "cdsscanner.h"

#include <functional>
#include <future>
#include <chrono>
#include <condition_variable>
#include <experimental/filesystem>
#include <chrono>
#include <thread>

#include "spdlog/spdlog.h"

#include "av/av.h"
#include "av/discid/discid.h"

#include "../common/utils.h"

#include "audio.h"
#include "ebook.h"

using namespace std::placeholders;
namespace fs = std::experimental::filesystem::v1;

namespace scanner {

FILE_TYPE parse ( const std::string& mime_type, const std::string& extension ) {
    if ( mime_type == "text/plain" ) {
        if ( strncasecmp ( extension.c_str(), ".log", 4 ) == 0 ) {
            //TODO check if logfile is valid
            return FILE_TYPE::logfile;

        } else if ( strncasecmp ( extension.c_str(), ".cue", 4 ) == 0 ) {
            return FILE_TYPE::cue;

        } else { return FILE_TYPE::file; }

    } else if ( mime_type.find ( "audio/" ) == 0 ) {
        return FILE_TYPE::audio;

    } else if ( mime_type.find ( "image/" ) == 0 ) {
        return FILE_TYPE::image;

    } else if ( mime_type.find ( "video/" ) == 0 ) {
        return FILE_TYPE::movie;

    } else if ( mime_type == "application/pdf" ) {
        return FILE_TYPE::ebook;

    } else if ( mime_type == "application/x-rar" ||
                mime_type == "application/CDFV2" ) {
        return FILE_TYPE::file;
    }

    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "unknown mime-type: {}, {}", mime_type, extension );
    return FILE_TYPE::file;
}

const char* _mime_type_audio = "audio/";
const char* _mime_type_video = "video/";
const char* _mime_type_image = "image/";
const char* _mime_type_pdf = "application/pdf";

CdsScanner::CdsScanner ( const std::string& redis, const std::string& redis_port ) :
    redis_ ( redis ), port_ ( std::stoi ( redis_port ) ),
    redox_config_ ( data::make_connection ( redis, std::stoi ( redis_port ), REDIS_DATABASE::CONFIG ) ) {

    _magic = magic_open ( MAGIC_MIME_TYPE );
    magic_load ( _magic, nullptr );

    if ( !sub_.connect ( redis, std::stoi ( redis_port ) ) ) {
        spdlog::get ( SCANNER_LOGGER )->warn ( "can not subscribe to redis queue" );

    } else {
        sub_.subscribe ( event::RESCAN, [this] ( const std::string& topic, const std::string& msg ) {
            event ( topic, msg );
        } );
    }
}

CdsScanner::~CdsScanner() {
    magic_close ( _magic );
}

void CdsScanner::event ( const std::string& chan, const std::string& msg ) {

    spdlog::get ( SCANNER_LOGGER )->info ( "START COMMAND:{}:{}", chan, msg );

    const data::redis_ptr redox_data_ =
        data::make_connection ( redis_, port_, REDIS_DATABASE::FS );

    if ( msg == "true" ) { //flush the database, when requested
        spdlog::get ( SCANNER_LOGGER )->info ( "flushdb: {}:{}/1", redis_, port_ );
        redox_data_->command ( { redis::FLUSHDB } );
    }

    //parse the directory tree for folders and files.
    redox::Command< std::vector< std::string > >& _c =
        redox_config_->commandSync< std::vector< std::string > > ( {redis::LRANGE, config::MEDIA, "0", "-1" } );

    if ( _c.ok() ) {
        for ( const std::string& __c : _c.reply() ) {
            redox_data_->command ( { redis::HMSET, data::make_key ( key::FS, data::hash ( __c ) ),
                                     param::CLASS, cls::FOLDER,
                                     param::NAME, filename ( __c, false ),
                                     param::PARENT, param::FILE,
                                     param::PATH, __c,
                                     param::TYPE, std::to_string ( static_cast< int > ( FILE_TYPE::folder ) ),
                                     param::TIMESTAMP, std::to_string ( data::time_millis() ) }  );

            redox_data_->command ( { redis::ZADD, data::make_key ( key::FS, key::ROOT, param::LIST ),
                                     int_str ( FILE_TYPE::folder ), data::hash ( __c ) } );

            spdlog::get ( SCANNER_LOGGER )->info ( "import folder, {}", __c );
            import_path ( redox_data_, __c, __c );
        }
    }

//    //import audiofiles
//    data::nodes ( redox_data_, static_cast< type_mask_t > ( FILE_TYPE::audio ), key::INDEX_FILES, [] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {
//        const std::string _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );
//        std::error_code _errc = Audio::import_file ( redis, key, _path );

//        if ( !!_errc )
//        { spdlog::get ( SCANNER_LOGGER )->warn ( "error in reading metadata: {}->{}", _errc.message(), _path );  }

//        return _errc;
//    } );

    spdlog::get ( SCANNER_LOGGER )->info ( "start importing metadata..." );

    //get metadata for folders
    redox::Command< std::vector< std::string > >& _c_metadata_folder =
        redox_data_->commandSync< std::vector< std::string > > ( {redis::ZRANGE, "fs:root:list", "0", "-1" } );

    if ( _c_metadata_folder.ok() ) {
        for ( const std::string& __c : _c_metadata_folder.reply() ) {
            metadata_folder ( redox_data_, __c );
        }
    }

    spdlog::get ( SCANNER_LOGGER )->info ( "END EVENT:{}:{}", chan, msg );
}

void CdsScanner::import_path ( data::redis_ptr redis, const std::string& parent_key, const std::string& path ) {

    type_mask_t _folder_type = FILE_TYPE::folder;

    //get files in folder
    fs::path _fs_path ( path );
    fs::directory_iterator end_itr;

    for ( fs::directory_iterator itr ( _fs_path ); itr != end_itr; ++itr ) {
        const std::string _item_filepath = path + "/" + itr->path().filename().string();
        const std::string _extension = itr->path().extension();
        const auto _ftime = fs::last_write_time ( itr->path() );
        const std::time_t _cftime = std::chrono::system_clock::to_time_t ( _ftime );

        if ( fs::is_regular_file ( itr->status() ) ) {
//TODO            if ( ! Scanner::timestamp ( redis, data::hash ( _item_filepath ),
//                                        static_cast<unsigned long> ( boost::filesystem::last_write_time ( itr->path() ) ) ) ) {
            const std::string _mime_type = mime_type ( _item_filepath );
            const FILE_TYPE _type = parse ( _mime_type, _extension );
            _folder_type |= _type;

            //store the file
            redis->command ( { redis::HMSET, data::make_key ( key::FS, data::hash ( _item_filepath ) ),
                               param::NAME,  filename ( itr->path().filename().string() ),
                               param::PARENT, data::hash ( parent_key ),
                               param::PATH, _item_filepath,
                               param::TYPE, int_str ( _type ),
                               param::CLASS, std::to_string ( _type ),
                               param::EXTENSION, itr->path().extension(),
                               param::SIZE, std::to_string ( fs::file_size ( itr->path() ) ),
                               param::MIME_TYPE, _mime_type,
                               param::FILE_TIME, std::asctime ( std::localtime ( &_cftime ) ),
                               param::TIMESTAMP, std::to_string ( data::time_millis() ) } );

            redis->command ( { redis::ZADD, data::make_key ( key::FS, data::hash ( parent_key ), param::LIST ),
                               std::to_string ( static_cast< type_mask_t > ( _type ) ),
                               data::hash ( _item_filepath )
                             } );
            redis->command ( { redis::ZADD, key::INDEX_FILES,
                               std::to_string ( static_cast< type_mask_t > ( _type ) ),
                               data::hash ( _item_filepath )
                             } );

        } else if ( fs::is_directory ( itr->status() ) ) {

            redis->command ( { redis::HMSET, data::make_key ( key::FS, data::hash ( _item_filepath ) ),
                               param::NAME, filename ( _item_filepath, false ),
                               param::PARENT, data::hash ( path ),
                               param::TYPE, std::to_string ( static_cast< type_mask_t > ( FILE_TYPE::folder ) ),
                               param::PATH, _item_filepath,
                               param::CLASS, std::to_string ( FILE_TYPE::folder ),
                               param::TIMESTAMP, std::asctime ( std::localtime ( &_cftime ) ) } );

            redis->command ( { redis::ZADD, data::make_key ( key::FS, data::hash ( parent_key ), param::LIST ),
                               std::to_string ( static_cast< type_mask_t > ( FILE_TYPE::folder ) ),
                               data::hash ( _item_filepath )
                             } );
            redis->command ( { redis::ZADD, key::INDEX_FILES,
                               std::to_string ( static_cast< type_mask_t > ( FILE_TYPE::folder ) ),
                               data::hash ( _item_filepath )
                             } );

            import_path ( redis, _item_filepath, _item_filepath );
        }
    }//loop folder contents

    redis->command ( { redis::HMSET, data::make_key ( key::FS, data::hash ( path ) ),
                       param::TYPE, std::to_string ( _folder_type ) } );
}

//std::error_code CdsScanner::import() {

//    //import audiofiles
//    redox::Command< std::vector< std::string > >& _c =
//        redox_data_->commandSync< std::vector< std::string > > ( { "ZRANGEBYSCORE", key::INDEX_FILES,
//                int_str ( FILE_TYPE::audio ), int_str ( FILE_TYPE::audio ) } );

//    if ( _c.ok() ) {
//        for ( const std::string& __c : _c.reply() ) {
//            const std::string _path = data::get ( redox_data_, data::make_key ( key::FS, __c ), param::PATH );
//            std::error_code _errc = Audio::import_file ( redox_data_, __c, _path );

//            if ( !!_errc )
//            { spdlog::get ( SCANNER_LOGGER )->warn ( "error in reading metadata: {}->{}", _errc.message(), _path );  }
//        }
//    }

//    //import music albums
//    data::nodes ( redox_data_, static_cast< type_mask_t > ( FILE_TYPE::folder ), key::INDEX_FILES, [] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {
//        const std::string _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );
//        const std::string _stype = data::get ( redis, data::make_key ( key::FS, key ), param::TYPE );
//        auto _type = parse ( _stype );

//        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Audio: {}->{}", _type, _path );

//        if ( ( _type & FILE_TYPE::logfile ) == FILE_TYPE::logfile ) {
//            auto _errc = Audio::mb_logfile ( redis, key );

//            if ( !_errc )
//            { return std::error_code(); }

//            SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "No metadata found for: {}->{}", key, _path );
//        }

//        if ( ( _type & FILE_TYPE::cue ) == FILE_TYPE::cue ) {

//        }

//        auto _errc = Audio::mb_files ( redis, key );

//        if ( !_errc )
//        { return std::error_code(); }

//        return std::error_code();
//    } );

//    data::nodes ( redox_data_, static_cast< type_mask_t > ( FILE_TYPE::movie ), key::INDEX_FILES, [] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {
//        const std::string _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );
//        const std::string _stype = data::get ( redis, data::make_key ( key::FS, key ), param::TYPE );
//        auto _type = parse ( _stype );

//        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Movie: {}->{}", _type, _path );

//        if ( ( _type & FILE_TYPE::movie ) == FILE_TYPE::movie ) {
//            //Audio::import_album ( redis, _type, key );
//        }
//        return std::error_code();
//    } );

//    //import ebooks
//    data::nodes ( redox_data_, static_cast< type_mask_t > ( FILE_TYPE::ebook ), key::INDEX_FILES, [] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {
//        const auto _type = parse ( data::get ( redis, data::make_key ( key::FS, key ), param::TYPE ) );
//        const auto _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );

//        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Ebook: {}->{}", _type, _path );
//        return std::error_code();
//    } );

//    return std::error_code();
//}

//void CdsScanner::metadata () {
//    redox::Command< std::vector< std::string > >& _c =
//        redox_data_->commandSync< std::vector< std::string > > ( {"ZRANGE", "fs:root:list", "0", "-1" } );

//    if ( _c.ok() ) {
//        for ( const std::string& __c : _c.reply() ) {
//            metadata_folder ( __c );
//        }
//    }
//}

std::error_code CdsScanner::metadata_folder ( data::redis_ptr redis, const std::string& key ) {

    std::error_code _errc;

    const FILE_TYPE _type = type ( redis, key );
    const auto _path = data::get ( redis, data::make_key ( "fs", key ), param::PATH );

    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Folder: {} ({:#018b}) {}", key, _type, _path );

    if ( ( _type & FILE_TYPE::audio ) == FILE_TYPE::audio ) {

        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Audio: {}->{}", _type, _path );

        std::vector< std::string > _audiofiles;

        //read the files in the folder
        redox::Command< std::vector< std::string > >& _c_metadata_folder =
            redis->commandSync< std::vector< std::string > > ( {redis::ZRANGE, data::make_key ( key::FS, key, param::LIST ), "0", "-1" } );

        if ( _c_metadata_folder.ok() ) {

            for ( const std::string& __c : _c_metadata_folder.reply() ) {
                const FILE_TYPE _file_type = type ( redis, __c );
                const auto _file_path = data::get ( redis, data::make_key ( key::FS, __c ), param::PATH );

                switch ( _file_type ) {
                case FILE_TYPE::audio:
                    if ( !! ( _errc = Audio::import_file ( redis, __c, _file_path ) ) )
                    { return _errc; }

                    _audiofiles.push_back ( filename ( _file_path ) );
                    break;

                case FILE_TYPE::ebook:
                    if ( !! ( _errc = EBook::import_file ( redis, __c, _file_path ) ) )
                    { return _errc; }

                    break;

                case FILE_TYPE::image:
                case FILE_TYPE::folder:
                case FILE_TYPE::file:
                    break;

                default:
                    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "FOLDER_CONTENT: {}{:#018b}->{}", __c, _file_type, _file_path );

                }
            }
        }

        discid::toc_t _toc = discid::parse_file ( _path, _audiofiles );
        std::cout << "=== TOC from files:\n" << _toc << std::endl;

        if ( ( _type & FILE_TYPE::logfile ) == FILE_TYPE::logfile ) {
            //TODO check with toc from files
            if ( ! ( _errc = Audio::mb_logfile ( redis, key ) ) ) {
                SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "metadata found in musicbrainz with logfile: {}->{}", key, _path );
                return std::error_code();
            }

            SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "No metadata found in musicbrainz with logfile: {}->{}", key, _path );
        }

        if ( ( _type & FILE_TYPE::cue ) == FILE_TYPE::cue ) {
            if ( ! ( _errc = Audio::mb_cue ( redis, key, _audiofiles ) ) ) {
                //TODO check with toc from files
                SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "metadata found in musicbrainz with cue: {}->{}", key, _path );
                return std::error_code();
            }

            SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "No metadata found in musicbrainz with cue: {}->{}", key, _path );
        }

        //save the files from files toc
        auto _errc = Audio::mb_files ( redis, key, _toc );

        if ( !_errc ) {
            SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "saved metadata from file: {}->{}", key, _path );
            return std::error_code();
        }

        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "no metadata found: {}->{}", key, _path );

    } else if ( ( _type & FILE_TYPE::ebook ) == FILE_TYPE::ebook ) {
        EBook::import_file ( redis, key, _path );

    } else if ( ( _type & FILE_TYPE::movie ) == FILE_TYPE::movie ) {

    } else {
        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Other folder: {} ({:#018b})->{}", key, static_cast< type_mask_t > ( _type ), _path );
    }

    //get the child elements
    data::nodes ( redis, static_cast< type_mask_t > ( FILE_TYPE::folder ), data::make_key ( "fs", key, "list" ), [this,&redis] ( data::redis_ptr, const std::string& key ) -> std::error_code {
        return metadata_folder ( redis, key );
    } );
    return _errc;
}



std::string CdsScanner::mime_type ( const std::string& filename ) {
    auto _magic = magic_open ( MAGIC_MIME_TYPE );
    magic_load ( _magic, nullptr );

    const char* _mime_type = magic_file ( _magic, filename.c_str() );

    if ( ! _mime_type ) {
        spdlog::get ( SCANNER_LOGGER )->warn ( "unable to load mime-type with libmagic: {}", filename );
        _mime_type = "text/plain";
    }

    return _mime_type;
}

FILE_TYPE CdsScanner::type ( data::redis_ptr redis, const std::string& key ) {
    auto& _type = redis->commandSync< std::string >
                  ( {redis::HGET, data::make_key ( key::FS, key ), param::TYPE } );

    if ( !_type.ok() ) {
        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "can not get type for: {}", data::make_key ( "fs", key ) );
        return FILE_TYPE::file;
    }

    return static_cast< FILE_TYPE > ( std::stoi ( _type.reply() ) );
}
}//namespace scanner
