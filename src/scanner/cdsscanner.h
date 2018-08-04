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
#ifndef APISERVER_H
#define APISERVER_H

//#include <memory>
//#include <mutex>
#include <string>
//#include <thread>

#include <magic.h>

#include "http/server.h"
#include "http/httpserver.h"

#include "../common/constants.h"
#include "../common/datastore.h"
#include "../common/squawkerrc.h"

namespace scanner {

const std::array< std::string, 6 > folder_type_names = {"unknown", "audio_image_cue", "audio_cue", "audio", "multidisc", "ebook" };
//enum class FOLDER_TYPE { UNKNOWN, AUDIO_IMAGE_CUE, AUDIO_CUE, AUDIO, AUDIO_MULTIDISC, EBOOK };
const std::array< std::string, 10 > type_names = {"unknown", "audio", "image", "ebook", "cue", "video", "subtitle", "directory", "disc", "artwork" };
enum class TYPE {REG_FILE, AUDIO, IMAGE, EBOOK, CUE_SHEET, VIDEO, SUBTITLE, DIRECTORY, DISC, ARTWORK };
///@cond DOC_INTERNAL
const std::vector< std::string > __NAMES {   param::FOLDER, param::AUDIO, param::MOVIE, param::SERIE, param::IMAGE, param::EBOOK,
          param::FILE, param::ALBUM, param::COVER, param::EPISODE, param::ARTIST };
///@endcond DOC_INTERNAL

typedef std::map< FILE_TYPE, int > type_list_t;
typedef const std::vector< std::regex > disc_matches_t;

///** @brief node type as string.  */
//static std::string to_string ( const FILE_TYPE type ) {
//    return __NAMES.at ( type );
//}
//}//namespace std

static std::ostream& operator<< ( std::ostream& stream, TYPE type ) {
    stream << type_names.at ( static_cast< unsigned short > ( type ) );
    return stream;
}

static std::ostream& operator<< ( std::ostream& stream, type_list_t types ) {
    stream << "types:[";
    bool is_first = true;

    for ( auto type : types ) {
        if ( is_first ) { is_first = false; }

        else { stream << "/"; }

        stream << type.first << "=" << type.second;
    }

    stream << "]";
    return stream;
}

/** scan the cntent of the filesystem */
class CdsScanner {
public:
    /** create a new scanner */
    CdsScanner ( const std::string& redis, const std::string& redis_port );
    ~CdsScanner();

private:
    const std::string redis_;
    const int port_;

    const data::redis_ptr redox_config_; //TODO remove
    redox::Subscriber sub_;

    magic_t _magic;

    /** start import; called from the event method */
    void event ( const std::string& chan, const std::string& msg );
    /** read the files from the path */
    void import_path ( data::redis_ptr redis, const std::string& parent_key, const std::string& path );
    /** read the metadata in the folder */
    std::error_code metadata_folder ( data::redis_ptr redis, const std::string& key );

    /** get the mime type with magic */
    std::string mime_type ( const std::string& filename );
    /** get the type bitmask of the folder */
    FILE_TYPE type ( data::redis_ptr redis, const std::string& key );
};
}//namespace scanner
#endif // APISERVER_H
