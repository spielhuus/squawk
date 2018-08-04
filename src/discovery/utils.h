#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <http/constant.h>
#include <sys/utsname.h>

namespace upnp {
inline namespace asio_impl {

inline std::string uname() {
    struct utsname uts;
    uname ( &uts );
    std::ostringstream system;
    system << uts.sysname << "/" << uts.version;
    return system.str();
};

inline std::string create_header ( std::string request_line, std::map< std::string, std::string > headers ) {
    std::ostringstream os;
    os << request_line + std::string ( "\r\n" );

    for ( auto & iter : headers ) {
        os << iter.first << ": " << iter.second << "\r\n";
    }

    os << "\r\n";
    return os.str();
}

/** @brief The SSDP Response */
struct SsdpResponse {

    SsdpResponse ( http::http_status status, std::string request_line, std::map< std::string, std::string > headers ) :
        status ( status ), request_line ( request_line ), headers ( headers ) {}

    http::http_status status;
    std::string request_line;
    std::map< std::string, std::string > headers;
};


/** @brief SSDP event item. */
struct SsdpEvent {
public:

    SsdpEvent ( const SsdpEvent& ) = default;
    SsdpEvent ( SsdpEvent&& ) = default;
    SsdpEvent& operator= ( const SsdpEvent& ) = default;
    SsdpEvent& operator= ( SsdpEvent&& ) = default;
    ~SsdpEvent() {}

    /**
      * Create the json stream.
      */
    friend std::ostream& operator<< ( std::ostream& out, const SsdpEvent & upnp_device ) {
        out << "{\"host\":\"" << upnp_device.host << "\",\"location\":\"" << upnp_device.location << "\",\"nt\":\"" << upnp_device.nt << "\"," <<
            "\"nts\":\"" <<  upnp_device.nts << "\",\"server\":\"" << upnp_device.server << "\",\"usn\":\"" << upnp_device.usn << "\"," <<
            "\"last_seen\":" << upnp_device.last_seen << ",\"cache_control\":" << upnp_device.cache_control << "}";
        return out;
    }

    std::string host;
    std::string location;
    std::string nt;
    std::string nts;
    std::string server;
    std::string usn;

    time_t last_seen;
    time_t cache_control;
};
}//namespace upnp
}//inline namespace asio_impl
#endif // UTILS_H
