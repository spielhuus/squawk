#ifndef SCANNER_H
#define SCANNER_H

#include <array>
#include <map>
#include <memory>
#include <regex>
#include <tuple>
#include <vector>

#include <magic.h>
#include <redox.hpp>

#include "../common/datastore.h"

const std::array< std::string, 6 > folder_type_names = {"unknown", "audio_image_cue", "audio_cue", "audio", "multidisc", "ebook" };
enum class FOLDER_TYPE { UNKNOWN, AUDIO_IMAGE_CUE, AUDIO_CUE, AUDIO, AUDIO_MULTIDISC, EBOOK };
const std::array< std::string, 10 > type_names = {"unknown", "audio", "image", "ebook", "cue", "video", "subtitle", "directory", "disc", "artwork" };
enum class TYPE {REG_FILE, AUDIO, IMAGE, EBOOK, CUE_SHEET, VIDEO, SUBTITLE, DIRECTORY, DISC, ARTWORK };
typedef std::map< TYPE, int > type_list_t;
typedef const std::vector< std::regex > disc_matches_t;

namespace std {
static const std::string to_string ( const FOLDER_TYPE& type ) {
    return folder_type_names.at ( static_cast< unsigned short > ( type ) );
}
static const std::string to_string ( const TYPE& type ) {
    return type_names.at ( static_cast< unsigned short > ( type ) );
}
}//namespace std

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

struct TreeItem {
    TreeItem ( TYPE type, const std::string& path, const size_t size, const time_t timstamp );
    ~TreeItem();

    const TYPE type;
    const size_t size;
    const time_t timestamp;
    const std::string path;
    std::vector< std::shared_ptr< TreeItem > > children;
    type_list_t contents;

};

class CdsScanner {
public:

    CdsScanner ( const std::string& redis, const std::string& redis_port );

    void init ( const std::string& redis, const std::string& redis_port );

    void rescan ( bool flush, std::function<void ( std::error_code& ) > fn );

    void flush();



    std::shared_ptr< TreeItem > parse ( const std::string& path );
    void visit ( TreeItem& item, std::function< void ( TreeItem& ) > fn );
    static FOLDER_TYPE type ( TreeItem& item );

private:
    static const char* _mime_type_audio;
    static const char* _mime_type_video;
    static const char* _mime_type_image;
    static const char* _mime_type_pdf;

    data::redis_ptr redox_;
    redox::Subscriber sub_;
    magic_t _magic;

    std::atomic<bool> rescanning_;


    std::unique_ptr< std::thread > scanner_thread_ = nullptr;

    void parse ( std::string& path );

    void parse ( TreeItem* parent, const std::string& path );
    const static std::vector< std::regex > _disc_patterns;
    const static std::vector< std::regex > _artwork_patterns;
    const static bool match ( disc_matches_t m, const std::string& path );

    void save_folder ( const std::string& key /** @param path path of the node. */,
                       const std::string& name /** @param name name of the node. */,
                       const std::string& parent /** @param parent parent key. */ );

};
#endif // SCANNER_H
