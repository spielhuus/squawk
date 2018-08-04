#ifndef SSDPSERVER_H
#define SSDPSERVER_H

#include <assert.h>

#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

#include "ssdpserverconnection.h"
#include "ssdpclientconnection.h"

#include "utils.h"

#include "../common/datastore.h"

namespace upnp {

/** @brief SSDP Server. */
class SSDPServerImpl {
public:
    /** @brief Create a new SSDPServer. */
    explicit SSDPServerImpl ( const std::string& redis, const std::string& redis_port );

    ~SSDPServerImpl();

    /**
     * Announce the services in the network.
     */
    void announce();
    /**
     * Suppress the services in the network.
     */
    void suppress();
    /**
     * Search for services in the network. The call is asynchronous, the services are notified.
     * @brief Search Services
     * @param service the service, default ssdp:all
     */
    void search ( const std::string & service = SSDP_NS_ALL );
    /**
    * Handle response callback method..
    * \param headers the responset headers
    */
    void handle_response ( http::Response & response );
    /**
    * Handle receive callback method..
    * \param headers the request headers
    */
    void handle_receive ( http::Request & request );


    SsdpEvent parseResponse ( http::Response & response );
    static inline time_t parse_keep_alive ( const std::string & cache_control ) {
        time_t time = 0;
        std::string cache_control_clean = boost::erase_all_copy ( cache_control, " " );

        if ( cache_control_clean.find ( SSDP_OPTION_MAX_AGE ) == 0 ) {
            time = boost::lexical_cast< time_t > ( cache_control_clean.substr ( SSDP_OPTION_MAX_AGE.size() ) );

        } else { assert ( false ); } //wrong cache control format

        return time;
    }

private:
    /** thread sleep time. */
    static const size_t SSDP_THREAD_SLEEP;
    /** hot many times the SSDP M-SEARCH and NOTIFY messages are sent. */
    static const size_t NETWORK_COUNT;
    /** hot many times the SSDP M-SEARCH and NOTIFY messages are sent. */
    static const size_t ANNOUNCE_INTERVAL;
    static const std::string SSDP_HEADER_SERVER, SSDP_HEADER_DATE, SSDP_HEADER_ST,
           SSDP_HEADER_NTS, SSDP_HEADER_USN, SSDP_HEADER_LOCATION, SSDP_HEADER_NT, SSDP_HEADER_MX,
           SSDP_HEADER_MAN, SSDP_HEADER_EXT, SSDP_OPTION_MAX_AGE, SSDP_REQUEST_LINE_OK,
           SSDP_STATUS_DISCOVER, SSDP_STATUS_ALIVE, SSDP_STATUS_BYE, SSDP_NS_ALL, SSDP_MSEARCH,
           SSDP_NOTIFY, SSDP_HEADER_REQUEST_LINE, SSDP_HEADER_SEARCH_REQUEST_LINE;


    data::redis_ptr redox_;
    redox::Subscriber sub_;

    std::string multicast_address_;
    std::string multicast_port_;
    std::string listen_address_;
    std::string listen_port_;
    std::string uuid_;

    std::unique_ptr<SSDPServerConnection> connection;

    SsdpEvent parseRequest ( http::Request & request );
    void send_anounce ( const std::string & nt, const std::string & location );
    void send_suppress ( const std::string & nt );

    std::map< std::string, std::string > namespaces();

    std::map< std::string, std::string > create_response ( const std::string & nt, const std::string & location );

    bool announce_thread_run = true;
    std::unique_ptr<std::thread> annouceThreadRunner;
    std::chrono::high_resolution_clock::time_point _announce_time;
    void annouceThread();

};
}//namespace upnp
#endif // SSDPSERVER_H
