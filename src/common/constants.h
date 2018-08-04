#ifndef SQUAWK_CONSTANTS_H
#define SQUAWK_CONSTANTS_H

#include <string>

namespace squawk {
static const char* VERSION = "0.0.0";
}//namespace squawk

namespace cls {
static const char* FOLDER = "folder";
static const char* ALBUM = "album";
}//namespace squawk

namespace event {
static const char* RESCAN = "cds::rescan";
}//namespace event

static const char* DISCOVERY_LOGGER = "discovery";
static const char* API_LOGGER = "api";
static const char* SCANNER_LOGGER = "scanner";

static const char* NS_ROOT = "root";
static const char* NS_ROOT_DEVICE = "upnp:rootdevice";
static const char* NS_MEDIASERVER = "urn:schemas-upnp-org:device:MediaServer:1";
static const char* NS_CONTENT_DIRECTORY = "urn:schemas-upnp-org:service:ContentDirectory:1";
static const char* NS_CONNECTION_MANAGER = "urn:schemas-upnp-org:service:ConnectionManager:1";
static const char* NS_MEDIA_RECEIVER_REGISTRAR = "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1";

namespace config {
static const  char* NAME = "config:name";
static const  char* MEDIA = "config:media";
static const  char* UUID = "config:discovery:uuid";
static const  char* API_LISTEN_ADDRESS = "config:api:listen_address";
static const  char* API_LISTEN_PORT = "config:api:listen_port";
static const  char* CMS_LISTEN_ADDRESS = "";
static const  char* CMS_LISTEN_PORT = "";
static const  char* UPNP_LISTEN_ADDRESS = "config:discovery:upnp:listen_address";
static const  char* UPNP_LISTEN_PORT = "config:discovery:upnp:listen_port";
static const  char* UPNP_MULTICAST_ADDRESS = "config:discovery:upnp:multicast_address";
static const  char* UPNP_MULTICAST_PORT = "config:discovery:upnp:multicast_port";
}//namespace config


typedef unsigned long type_mask_t;

enum FILE_TYPE {
    folder = ( 1u << 0 ), audio = ( 1u << 1 ), movie = ( 1u << 2 ), serie = ( 1u << 3 ),
    image = ( 1u << 4 ), ebook = ( 1u << 5 ), file = ( 1u << 6 ), album = ( 1u << 7 ),
    cover = ( 1u << 8 ), episode = ( 1u << 9 ), artist = ( 1u << 10 ), cue = ( 1u << 11 ),
    logfile = ( 1u << 12 )
};

static FILE_TYPE parse ( const std::string& type ) {
    const int _i_type = std::stoi ( type );
    return static_cast< FILE_TYPE > ( _i_type );
};

static std::string int_str ( const FILE_TYPE& type ) {
    return std::to_string ( static_cast< type_mask_t > ( type ) );
}

namespace std {
static const std::string to_string ( const FILE_TYPE& type ) {
    if ( type == FILE_TYPE::folder )
    { return "folder"; }

    if ( type == FILE_TYPE::audio )
    { return "audio"; }

    if ( type == FILE_TYPE::movie )
    { return "movie"; }

    if ( type == FILE_TYPE::serie )
    { return "serie"; }

    if ( type == FILE_TYPE::image )
    { return "image"; }

    if ( type == FILE_TYPE::ebook )
    { return "ebook"; }

    if ( type == FILE_TYPE::album )
    { return "album"; }

    if ( type == FILE_TYPE::cover )
    { return "cover"; }

    if ( type == FILE_TYPE::episode )
    { return "episode"; }

    if ( type == FILE_TYPE::artist )
    { return "artist"; }

    if ( type == FILE_TYPE::cue )
    { return "cue"; }

    if ( type == FILE_TYPE::logfile )
    { return "logfile"; }

    return "file";
}
}//namespace std

/** @brief Object ID */
static const  char* OBJECT_ID = "ObjectID";
static const  char* BROWSE_FLAG = "BrowseFlag";
static const  char* BROWSE_METADATA = "BrowseMetadata";
static const  char* BROWSE_DIRECT_CHILDREN = "BrowseDirectChildren";
static const  char* BROWSE_RESPONSE = "BrowseResponse";

namespace key {
static const char* FS           = "fs";
static const char* ARTIST       = "artist";
static const char* KEY          = "key";
static const char* ROOT         = "root";
static const char* INDEX        = "index";
static const char* INDEX_FILES  = "index:files";
}//namespace key

namespace param {
static const char* ALBUM          = "album";
static const char* ARTIST         = "artist";
static const char* AUTHOR         = "author";
static const char* AUDIO          = "audio";
static const char* BACKDROP_PATH  = "backdrop_path";
static const char* BITRATE        = "bitrate";
static const char* BPS            = "bps";
static const char* CHANNELS       = "channels";
static const char* CLASS          = "cls";
static const char* CLEAN_STRING   = "clean_string";
static const char* COMMENT        = "comment";
static const char* COMPOSER       = "composer";
static const char* COVER          = "cover";
static const char* DATE           = "date";
static const char* DISC           = "disc";
static const char* EBOOK          = "ebook";
static const char* EPISODE        = "episode";
static const char* EXT            = "ext";
static const char* EXTENSION      = "ext";
static const char* FILE           = "file";
static const char* FILE_TIME      = "file-time";
static const char* FOLDER         = "folder";
static const char* GENRE          = "genre";
static const char* HEIGHT         = "height";
static const char* HOMEPAGE       = "homepage";
static const char* IMDB_ID        = "imdb_id";
static const char* IMAGE          = "image";
static const char* ISBN           = "isbn";
static const char* LANGUAGE       = "language";
static const char* LIST           = "list";
static const char* MAKE           = "Make";
static const char* MBID           = "mbid";
static const char* MED            = "med";
static const char* MIME_TYPE      = "mimeType";
static const char* MOVIE          = "movie";
static const char* NAME           = "name";
static const char* PARENT         = "parent";
static const char* PATH           = "path";
static const char* PERFORMER      = "performer";
static const char* PLAYTIME       = "playlength";
static const char* POSTER_PATH    = "poster_path";
static const char* PUBLISHER      = "publisher";
static const char* ROOT           = "root";
static const char* SAMPLERATE     = "samplerate";
static const char* SEASON         = "season";
static const char* SERIE          = "serie";
static const char* SIZE           = "size";
static const char* STILL_IMAGE    = "still_image";
static const char* THUMB          = "thumb";
static const char* TIMESTAMP      = "timestamp";
static const char* TITLE          = "title";
static const char* TMDB_ID        = "tmdb_id";
static const char* TRACK          = "track";
static const char* TYPE           = "type";
static const char* WIDTH          = "width";
static const char* YEAR           = "year";
}//namespace param

#endif // SQUAWK_CONSTANTS_H
