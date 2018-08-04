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
#ifndef AUDIO_H
#define AUDIO_H

#include <string>
#include <system_error>

#include "av/discid/discid.h"

#include "../common/constants.h"
#include "../common/datastore.h"

namespace scanner {

class Audio {
public:
    static std::error_code import_file ( /** Redis database pointer. */  data::redis_ptr redis,
            /** key of the album. */        const std::string& key,
            /** path of the album. */       const std::string& path );

//    static std::error_code import_album ( /** Redis database pointer. */  data::redis_ptr redis,
//            /** file type of folder. */     const FILE_TYPE& type,
//            /** key of the album. */        const std::string& key );

    static std::error_code mb_logfile ( data::redis_ptr redis, const std::string& key );
    static std::error_code mb_files ( data::redis_ptr redis, const std::string& key, discid::toc_t& songs_toc );
    static std::error_code mb_cue ( data::redis_ptr redis, const std::string& key, std::vector< std::string >& audiofiles );

private:
    Audio();

    static std::error_code save ( data::redis_ptr redis, const std::string& key, const discid::toc_t& toc );

    static std::error_code mb ( data::redis_ptr redis, const std::string& key, const std::string& uri );
};
}//namespace scanner
#endif // AUDIO_H
