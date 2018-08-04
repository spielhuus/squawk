#ifndef DIDL_H
#define DIDL_H

#include <string>
#include <map>

#include "rapidxml_ns.hpp"
#include "rapidxml_ns_print.hpp"
#include "rapidxml_ns_utils.hpp"

#include "../common/constants.h"
#include "../common/datastore.h"

namespace upnp {

/** @brief DIDL root node name */
const static std::string DIDL_ROOT_NODE = "DIDL-Lite";
/** @brief DIDL container name */
const static std::string DIDL_ELEMENT_CONTAINER = "container";
/** @brief DIDL item name */
const static std::string DIDL_ELEMENT_ITEM = "item";
/** @brief DIDL element class */
const static std::string DIDL_ELEMENT_CLASS = "class";
/** @brief DIDL TITLE */
const static std::string DIDL_ELEMENT_TITLE = "title";
/** @brief DIDL id attribute name */
const static std::string DIDL_ATTRIBUTE_ID = "id";
/** @brief DIDL parent id attribute name */
const static std::string DIDL_ATTRIBUTE_PARENT_ID = "parentID";
/** @brief DIDL restricted attribute name */
const static std::string DIDL_ATTRIBUTE_RESTRICTED = "restricted";
/** @brief DIDL child count attribute name */
const static std::string DIDL_ATTRIBUTE_CHILD_COUNT = "childCount";
/** @brief DIDL album art uri */
const static std::string DIDL_ALBUM_ART_URI = "albumArtURI";

/** @brief XML DIDL NAMESPACE */
const static std::string XML_NS_DIDL = "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/";
/** @brief XML PURL NAMESPACE */
const static std::string XML_NS_PURL = "http://purl.org/dc/elements/1.1/";
/** @brief XML DLNA NAMESPACE */
const static std::string XML_NS_DLNA = "urn:schemas-dlna-org:metadata-1-0/";
/** @brief XML DLNA Metadata NAMESPACE */
const static std::string XML_NS_DLNA_METADATA = "urn:schemas-dlna-org:metadata-1-0/";
/** @brief XML PV NAMESPACE */
const static std::string XML_NS_PV = "http://www.pv.com/pvns/";
/** @brief XML UPNP NAMESPACE */
const static std::string XML_NS_UPNP = "urn:schemas-upnp-org:metadata-1-0/upnp/";

///@cond DOC_INTERNAL
namespace _internal {
static const std::vector< std::string > __NAMES = std::vector< std::string> (
{   param::FOLDER, param::AUDIO, param::MOVIE, param::SERIE, param::IMAGE, param::EBOOK,
    param::FILE, param::ALBUM, param::COVER, param::EPISODE, param::ARTIST } );
}
///@endcond DOC_INTERNAL

class NodeType {
public:
    enum Enum { folder = 0, audio = 1, movie = 2, serie = 3, image = 4, ebook = 5, file = 6, album = 7, cover = 8, episode = 9, artist = 10 };

    /** @brief node type as string.  */
    static std::string str ( const Enum type ) {
        return _internal::__NAMES.at ( type );
    }

    /** @brief get type from string.  */
    static Enum parse ( const std::string& type ) {
        if ( type == param::FOLDER )
        { return folder; }

        if ( type == param::AUDIO )
        { return audio; }

        if ( type == param::MOVIE )
        { return movie; }

        if ( type == param::SERIE )
        { return serie; }

        if ( type == param::IMAGE )
        { return image; }

        if ( type == param::EBOOK )
        { return ebook; }

        if ( type == param::ALBUM )
        { return album; }

        if ( type == param::COVER )
        { return cover; }

        if ( type == param::ARTIST )
        { return artist; }

        if ( type == param::EPISODE )
        { return episode; }

        return file;
    }

    /** @brief is the type string valid.  */
    static bool valid ( const std::string& type ) {
        return std::find ( _internal::__NAMES.begin(), _internal::__NAMES.end(), type ) != _internal::__NAMES.end();
    }
};

/** @brief write didl metadata to xml. */
struct Didl {
public:
    /** @brief Didl CTOR. */
    Didl ( data::redis_ptr redis );
    /** @brief Didl DTOR. */
    ~Didl() {}
    void write ( const std::string& key, const std::map< std::string, std::string >& values );

    /** @brief write xml to string. */
    std::string str() {
        std::string s = "<?xml version=\"1.0\"?>\n";
        rapidxml_ns::print ( std::back_inserter ( s ), doc_, rapidxml_ns::print_no_indenting );
        return s;
    }
    /** @brief number of objects written to xml */
    int count() const { return result_; }
private:
    static const std::array< std::string, 11 > CLASS_NAMES;
    static const std::array< upnp::NodeType::Enum, 4 > CLASS_CONTAINER;

    data::redis_ptr redis_;

    int result_;
    rapidxml_ns::xml_document<> doc_;
    rapidxml_ns::xml_node<>* root_node_;
};
}//namespace upnp
#endif // DIDL_H
