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
#include "audio.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "spdlog/spdlog.h"

#include "http/httpclient.h"

#include "av/av.h"

#include "../common/utils.h"

namespace scanner {

/** get the audio files from the node */
inline std::vector< std::string > audiofiles ( data::redis_ptr redis, const std::string& key, bool full_path = false ) {

    std::vector< std::string > _files;

    data::nodes ( redis, FILE_TYPE::audio, data::make_key ( key::FS, key, param::LIST ),
    [&_files,&full_path] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {

        const std::string _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );

        if ( full_path ) {
            _files.push_back ( _path );

        } else {
            _files.push_back ( filename ( _path ) );
        }
        return std::error_code();
    } );
    std::sort ( _files.begin(), _files.end() );
    return _files;
}

Audio::Audio() {}

std::error_code Audio::import_file ( data::redis_ptr redis, const std::string& key, const std::string& path ) {

    av::Format _format ( path );

    if ( !_format ) {
        auto _metadata = _format.metadata();
        auto _codec =  std::find_if ( _format.begin(), _format.end(), av::is_audio );

        if ( _codec != _format.end() ) {

            //save audio parameters
            redis->command ( { redis::HMSET, data::make_key ( key::FS, key ),
                               param::NAME, _metadata.get ( "title" ),
                               param::ARTIST, _metadata.get ( "artist" ),
                               param::BITRATE, std::to_string ( ( *_codec )->bitrate() ),
                               param::CHANNELS, std::to_string ( ( *_codec )->channels() ),
                               param::COMPOSER, _metadata.get ( "composer" ),
                               param::DATE, _metadata.get ( "date" ),
                               param::DISC, _metadata.get ( "disc" ),
                               param::GENRE, _metadata.get ( "genre" ),
                               param::TRACK, _metadata.get ( "track" ),
                               param::SAMPLERATE, std::to_string ( ( *_codec )->sample_rate() ),
                               param::PLAYTIME, std::to_string ( _format.playtime() )
                             } );

        } else { return _format.errc(); }

    } else { return _format.errc(); }

    return std::error_code();
}

std::error_code Audio::mb_logfile ( data::redis_ptr redis, const std::string& key ) {
    const std::string _album_path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );

    return data::nodes ( redis, FILE_TYPE::logfile, data::make_key ( key::FS, key, param::LIST ), [] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {
        const std::string _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );
        std::stringstream _logfile_stream;
        discid::convert ( _path, _logfile_stream );

        try {
            auto songs = discid::parse_logfile ( _logfile_stream );

            std::error_code _errc;

            if ( ! songs.empty() ) {
                discid::release_t _query_result;

                if ( ! ( _errc = discid::mb ( songs, _query_result ) ) ) {
                    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Album from logfile, musicbrainz: {}", _query_result.size() );
                    discid::toc_t _lookup_result;

                    auto _result_item = _query_result.front();

                    if ( ! ( _errc = discid::mb ( _result_item.mbid, _lookup_result ) ) ) {
                        //save the toc
                        save ( redis, key, _lookup_result );
                    }

                } else if ( ! ( _errc = discid::cddb ( songs, _query_result ) ) ) {
                    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Album from logfile, cddb: {}", _query_result.size() );
                    discid::toc_t _lookup_result;

                    auto _result_item = _query_result.front();

                    if ( ! ( _errc = discid::cddb ( _result_item.category, _result_item.mbid, _lookup_result ) ) ) {
                        //save the toc
                        save ( redis, key, _lookup_result );
                    }
                }

            } else {
                SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "No songs in logfile: {}", _path );
                _errc = av::make_error_code ( av::AV_HTTP_NOT_FOUND );
            }

            return _errc;

        } catch ( ... ) {
            spdlog::get ( SCANNER_LOGGER )->warn ( "Exception in mb_logile: {}", key );
            return av::make_error_code ( av::AV_UNKNOWN );
        }
    } );
}

std::error_code Audio::mb_cue ( data::redis_ptr redis, const std::string& key, std::vector< std::string >& audiofiles ) {
    const std::string _album_path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );

    return data::nodes ( redis, FILE_TYPE::logfile, data::make_key ( key::FS, key, param::LIST ), [&audiofiles] ( data::redis_ptr redis, const std::string& key ) -> std::error_code {
        const std::string _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );
        std::stringstream _logfile_stream;
        discid::convert ( _path, _logfile_stream );

        try {
            auto songs = discid::parse_cuesheet ( _logfile_stream, _path, audiofiles );

            std::error_code _errc;

            if ( ! songs.empty() ) {
                discid::release_t _query_result;

                if ( ! ( _errc = discid::mb ( songs, _query_result ) ) ) {
                    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Album from cuesheet, musicbrainz: {}", _query_result.size() );
                    discid::toc_t _lookup_result;

                    auto _result_item = _query_result.front();

                    if ( ! ( _errc = discid::mb ( _result_item.mbid, _lookup_result ) ) ) {
                        //save the toc
                        save ( redis, key, _lookup_result );
                    }

                } else if ( ! ( _errc = discid::cddb ( songs, _query_result ) ) ) {
                    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Album from cuesheet, cddb: {}", _query_result.size() );
                    discid::toc_t _lookup_result;

                    auto _result_item = _query_result.front();

                    if ( ! ( _errc = discid::cddb ( _result_item.category, _result_item.mbid, _lookup_result ) ) ) {
                        //save the toc
                        save ( redis, key, _lookup_result );
                    }
                }

            } else {
                SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "No songs in cuesheet: {}", _path );
                _errc = av::make_error_code ( av::AV_HTTP_NOT_FOUND );
            }

            return _errc;

        } catch ( ... ) {
            spdlog::get ( SCANNER_LOGGER )->warn ( "Exception in cddb_cuesheet: {}", key );
            return av::make_error_code ( av::AV_UNKNOWN );
        }
    } );
}

std::error_code Audio::mb_files ( data::redis_ptr redis, const std::string& key, discid::toc_t& songs_toc ) {
    try {
        std::error_code _errc;

        if ( ! songs_toc.empty() ) {
            discid::release_t _query_result;

            if ( ! ( _errc = discid::mb ( songs_toc, _query_result ) ) ) {
                SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Album from cuesheet, musicbrainz: {}", _query_result.size() );
                discid::toc_t _lookup_result;

                auto _result_item = _query_result.front();

                if ( ! ( _errc = discid::mb ( _result_item.mbid, _lookup_result ) ) ) {
                    //save the toc
                    save ( redis, key, _lookup_result );
                }

            } else if ( ! ( _errc = discid::cddb ( songs_toc, _query_result ) ) ) {
                SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "Album from cuesheet, cddb: {}", _query_result.size() );
                discid::toc_t _lookup_result;

                auto _result_item = _query_result.front();

                if ( ! ( _errc = discid::cddb ( _result_item.category, _result_item.mbid, _lookup_result ) ) ) {
                    //save the toc
                    save ( redis, key, _lookup_result );
                }
            }

        } else {
            SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "No songs in cuesheet: {}", key );
            _errc = av::make_error_code ( av::AV_HTTP_NOT_FOUND );
        }

        return _errc;

    } catch ( ... ) {
        spdlog::get ( SCANNER_LOGGER )->warn ( "Exception in import_file: {}", key );
        return av::make_error_code ( av::AV_UNKNOWN );
    }
}

inline std::error_code parse_release ( data::redis_ptr redis, json& __release, const std::string& _path, const std::string& key ) {
    const auto _release_id = __release[ "id" ].get< std::string >();
    const auto _release_title = __release[ "title" ].get< std::string >();

    std::regex base_regex ( _release_title, std::regex::icase );
    std::smatch base_match;

    if ( std::regex_search ( _path, base_match, base_regex ) ) {

        std::string _url = "http://musicbrainz.org/ws/2/release/";
        _url.append ( _release_id );
        _url.append ( "?inc=artist-credits+labels+discids+recordings&fmt=json" );

        std::stringstream _ss_release;
        std::this_thread::sleep_for ( std::chrono::seconds ( 2 ) );
        auto response = http::get ( _url, _ss_release );
        //TODO check status
        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "HTTP STATUS: {}->{}", static_cast< int > ( response.status() ), _url );

        auto _json_release = json::parse ( _ss_release.str() );

        std::vector< std::string > _command ( {
            redis::HMSET, data::make_key ( key::FS, key ),
            param::TYPE, std::to_string ( static_cast< type_mask_t > ( FILE_TYPE::audio ) ),
            param::CLASS, cls::ALBUM
        } );

        if ( _json_release.find ( "title" ) != _json_release.end() ) {
            const std::string _title = _json_release["title"];
            _command.push_back ( param::TITLE );
            _command.push_back ( _title );
        }

        if ( _json_release.find ( "id" ) != _json_release.end() ) {
            const std::string _id = _json_release["id"];
            _command.push_back ( "mbid" );
            _command.push_back ( _id );
        }

        if ( _json_release.find ( "date" ) != _json_release.end() ) {
            const std::string _date = _json_release["date"];
            _command.push_back ( param::DATE );
            _command.push_back ( _date );
        }

        if ( _json_release["country"].is_string() ) {
            const std::string _country = _json_release["country"];
            _command.push_back ( "country" );
            _command.push_back ( _country );
        }

        for ( auto& __media : _json_release["media"] ) {
            auto tracks = audiofiles ( redis, data::make_key ( key::FS, key ), true );

            for ( auto& __track : __media["tracks"] ) {

                const auto _position = __track["position"].get< int >();
                const auto _length = __track["length"].get< int >();
                const auto _title = __track["title"].get< std::string >();
                const auto _id = __track["id"].get< std::string >();

                if ( tracks.size() > _position ) {
                    redis->command ( { redis::HMSET, data::make_key ( key::FS, data::hash ( tracks.at ( _position ) ) ),
                                       param::TRACK, std::to_string ( _position ),
                                       param::TITLE, _title,
                                       param::MBID, _id }  );
                }

                for ( auto& __artist_credit : __track["artist-credit"] ) {

                    auto& __artist = __artist_credit["artist"];
                    const auto _artist_id = __artist["id"].get< std::string >();
                    const auto _artist_name = __artist["name"].get< std::string >();
                    const auto _artist_sort_name = __artist["sort-name"].get< std::string >();

                    redis->command ( { redis::HMSET, data::make_key ( key::ARTIST, _artist_name ),
                                       param::NAME, _artist_name,
                                       "sort-name", _artist_sort_name,
                                       param::MBID, _artist_id }  );

                    redis->command ( { redis::ZADD,
                                       data::make_key ( key::ARTIST, _artist_name, "albums" ),
                                       "0", key } );
                    redis->command ( { redis::ZADD,
                                       data::make_key ( key::FS, key, "artists" ),
                                       "0",
                                       _artist_name } );
                }
            }
        }

        redis->command ( _command );
        redis->command ( { redis::ZADD, "index:files",  std::to_string ( static_cast< type_mask_t > ( FILE_TYPE::album ) ), key  } );

        return std::error_code();

    } else {
        SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "no match: {}", _release_title );
        return av::make_error_code ( av::AV_EXIT );
    }
}
std::error_code Audio::mb ( data::redis_ptr redis, const std::string& key, const std::string& uri ) {

    const auto _path = data::get ( redis, data::make_key ( key::FS, key ), param::PATH );

    std::stringstream _ss;
    std::this_thread::sleep_for ( std::chrono::seconds ( 2 ) );
    auto response = http::get ( uri, _ss );

    if ( response.status() == http::http_status::OK ) {

        auto _json = json::parse ( _ss.str() );

        if ( _json.find ( "releases" ) != _json.end() && _json.find ( "releases" )->size() > 0 )  {
            for ( auto& __release : _json["releases"] ) {
                std::error_code _errc = parse_release ( redis, __release, _path, key );

                if ( !_errc )
                {return std::error_code(); }
            }

        } else {

            if ( _json.find ( "releases" ) == _json.end() )  {//ensure that there is not an empty releases array
                std::error_code _errc = parse_release ( redis, _json, _path, key );

                if ( !_errc )
                {return std::error_code(); }
            }
        }
    }

    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "HTTP: {}->{}", static_cast< int > ( response.status() ), uri );
    return av::make_error_code ( av::AV_EXIT );
}

std::error_code Audio::save ( data::redis_ptr redis, const std::string& key, const discid::toc_t& toc ) {
    std::error_code _errc;

    if ( !toc.empty() ) {

        auto _metadata = toc.front().metadata;

        //save album
        redis->command ( { redis::HMSET, data::make_key ( key::FS, key ),
                           param::NAME, _metadata.get ( "album" ),
                           param::ARTIST, _metadata.get ( "artist" ),
                           param::DATE, _metadata.get ( "date" ),
                           param::GENRE, _metadata.get ( "genre" ),
                         } );

        auto _item_iterator = toc.begin();

        for ( auto& file : audiofiles ( redis, key, true ) ) {
            auto _metadata = _item_iterator->metadata;
            //save album
            redis->command ( { redis::HMSET, data::make_key ( key::FS, data::hash ( file ) ),
                               param::TITLE, _metadata.get ( "name" ),
                               param::ALBUM, _metadata.get ( "album" ),
                               param::ARTIST, _metadata.get ( "artist" ),
                               param::DATE, _metadata.get ( "date" ),
                               param::GENRE, _metadata.get ( "genre" ),
                               param::TRACK, _metadata.get ( "track" ),
                               param::DISC, _metadata.get ( "disc" ),
                             } );

        }
    }

    return _errc;
}

}//namespace scanner
