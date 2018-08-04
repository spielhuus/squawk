/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DATASTORE_H
#define DATASTORE_H

#include <functional>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/functional/hash.hpp>

#include "fmt/format.h"
#include <redox.hpp>

#include "gtest/gtest_prod.h"

enum class REDIS_DATABASE {FS=0, CONFIG=1};

/** the redis commands */
namespace redis {
static const std::string SADD       = "SADD";
static const std::string DEL        = "DEL";
static const std::string EXISTS     = "EXISTS";
static const std::string FLUSHDB    = "FLUSHDB";
static const std::string HGET       = "HGET";
static const std::string HGETALL    = "HGETALL";
static const std::string HMSET      = "HMSET";
static const std::string LRANGE     = "LRANGE";
static const std::string SCARD      = "SCARD";
static const std::string SMEMBERS   = "SMEMBERS";
static const std::string SUNION     = "SUNION";
static const std::string SREM       = "SREM";
static const std::string ZADD       = "ZADD";
static const std::string ZCARD      = "ZCARD";
static const std::string ZRANGE     = "ZRANGE";
static const std::string ZREVRANGE  = "ZREVRANGE";
static const std::string ZREM       = "ZREM";

static const std::string FT_ADD     = "FT.ADD";
static const std::string FT_CREATE  = "FT.CREATE";
static const std::string FT_DROP    = "FT.DROP";
static const std::string FT_SEARCH  = "FT.SEARCH";
static const std::string FT_SUGGADD = "FT.SUGADD";
}

/////@cond DOC_INTERNAL
/////@endcond DOC_INTERNAL

/** @brief Read and write CDS data in redis. */
struct data {

    typedef std::shared_ptr< redox::Redox > redis_ptr;
    typedef std::map< std::string, std::string > node_t;
    typedef std::set< std::string > nodes_t;
    typedef std::function< void ( const std::string& ) > async_fn;
    typedef std::vector< std::string > command_t;

    /** @brief current timestamp in millis. */
    static unsigned long time_millis() {
        return static_cast<unsigned long> (
                   std::chrono::system_clock::now().time_since_epoch() /
                   std::chrono::milliseconds ( 1 ) );
    }
    /** @brief hash create a hash of the input string. */
    static std::string hash ( const std::string& in /** @param in string to hash. */ ) {
        static boost::hash<std::string> _hash;
        return std::to_string ( _hash ( in ) );
    }

    template< typename... ARGS >
    /** @brief make_key */
    static std::string make_key (
        const std::string& prefix, /** @param prefix the prefix of the string*/
        const std::string& value, /** @param value the first value to add */
        ARGS... args /** @param args the following tokens */ ) {
        std::string _res = prefix;
        __iterate_key ( _res, value, args... );
        return _res;
    }

    static std::string type ( redis_ptr redis /** @param redis the database pointer. */,
                              const std::string& key /** @param key redis key. */ ) {

        auto& _item = redis->commandSync< std::string > ( {"type", key} );
        /** @return the the value or empty string if not found. */
        return ( _item.ok() ? _item.reply() : "" );
    }

// -----------------------------------------------------------------------------------------------------------
// --------------------------                    database utils                     --------------------------
// -----------------------------------------------------------------------------------------------------------

    /** @brief create new database connection. */
    static redis_ptr make_connection (
        /** database host */ const std::string db,
        /** database port */ int port,
        /** select database */ REDIS_DATABASE database = REDIS_DATABASE::FS ) {

        redis_ptr _redis = redis_ptr ( new redox::Redox ( std::cout, redox::log::Warning ) );

        if ( !_redis->connect ( db, port ) ) {
            throw std::system_error ( std::error_code ( 300, std::generic_category() ),
                                      "unable to connect to database on localhost." );
        }

        _redis->commandSync ( {"SELECT", std::to_string ( static_cast< int > ( database ) ) } );
        return _redis;
    }

    /** @brief get the node attribute value. */
    static std::string get ( redis_ptr redis /** @param redis the database pointer. */,
                             const std::string& key /** @param key redis key. */,
                             const std::string param /** @param param parameter name. */ ) {

        auto& _item = redis->commandSync< std::string > ( {redis::HGET, key, param} );
        /** @return the the value or empty string if not found. */
        return ( _item.ok() ? _item.reply() : "" );
    }

//    /** @brief store the node in redis. */
//    static void save ( redis_ptr redis /** @param redis pointer to redis database. */,
//                       const std::string& key /** @param key key of the object to store. */,
//                       const node_t& node /** @param node the node values to store. */ ) {
//        auto _command = std::vector< std::string > ( { redis::HMSET, make_key ( "fs", key ) } );

//        for ( auto __i : node ) {
//            _command.push_back ( __i.first );
//            _command.push_back ( __i.second );
//        }

//        redis->command ( _command );
//    }

    /** @brief get the node by key TODO REMOVE */
    static node_t node ( redis_ptr redis /** @param redis redis database pointer. */,
                         const std::string& key /** @param key the node key. */ ) {

        auto& _item = redis->commandSync< std::vector< std::string > > ( { redis::HGETALL, make_key ( "fs", key ) } );
        /** @return the the node or empty map if not found. */
        return ( _item.ok() ? to_map ( _item.reply() ) : node_t() );
    }

    /** @brief get the node children */
    static void children ( redis_ptr redis /** @param redis redis database pointer. */,
                           const std::string& key /** @param key the node key. */,
                           const int& index /** @param index start index. */,
                           const int& count /** @param count result size. */,
                           const std::string& sort /** @param sort sort results [alpha, timestamp]. */,
                           const std::string& order /** @param order order the results [asc, desc]. */,
                           const std::string& filter /** @param filter filter results by keyword. */,
                           async_fn fn /** @param fn the callback function. */ ) {

        //TODO sort and filter


        int _end_pos = ( count > 0 ? index + count -1 : count );
        command_t _redis_command;

        if ( !filter.empty() && filter != "*" ) {
            _redis_command = { redis::FT_SEARCH, "index", filter, "NOCONTENT", "LIMIT", std::to_string ( index ), std::to_string ( index + count - 1 ) };

        } else if ( sort == "default" ) {
            _redis_command = { ( order=="desc"?redis::ZREVRANGE:redis::ZRANGE ), make_key ( "fs", key, "list" ), std::to_string ( index ), std::to_string ( _end_pos ) };

        } else {
            _redis_command = { redis::LRANGE, make_key ( make_key ( "fs", key, "list" ), "sort", sort, order ), std::to_string ( index ), std::to_string ( index + count - 1 ) };
        }

        redox::Command< std::vector< std::string > >& _c = redis->commandSync< std::vector< std::string > > ( _redis_command );

        if ( _c.ok() ) {
            for ( const std::string& __c : _c.reply() ) {
                fn ( __c );
            }
        }
    }

//    /** @brief get the node files by type*/
//    static void files ( redis_ptr redis /** @param redis redis database pointer. */,
//                        const std::string& key /** @param key the file key. */,
//                        const NodeType::Enum type /** @param type the file type. */,
//                        const int& index /** @param index start index. */,
//                        const int& count /** @param count result size. */,
//                        async_fn fn /** @param fn the callback function. */ ) {

//        redox::Command< std::vector< std::string > >& _c = redis->commandSync< std::vector< std::string > > (
//        { redis::ZRANGE, data::make_key ( key::FS, key, "types" /** TODO */, NodeType::str ( type ) ), std::to_string ( index ), std::to_string ( index + count - 1 ) }
//        );

//        if ( _c.ok() ) {
//            for ( const std::string& __c : _c.reply() ) {
//                fn ( __c );
//            }
//        }
//    }

//    /** @brief get the node children count */ //TODO add params sort,order,tags
//    static int children_count ( redis_ptr redis, const std::string& key ) {
//        redox::Command< int >& _c = redis->commandSync< int > (
//        { redis::ZCARD, make_key_list ( key ) }
//        );
//        return ( _c.ok() ? _c.reply() : 0 );
//    }

//    /** @brief get the node children count */ //TODO add params sort,order,tags
//    static int files_count ( redis_ptr redis, const std::string& key, const NodeType::Enum type ) {
//        redox::Command< int >& _c = redis->commandSync< int > (
//        { redis::ZCARD, make_key ( key::FS, key, "types", NodeType::str ( type ) ) }
//        );
//        return ( _c.ok() ? _c.reply() : 0 );
//    }

//    /** @brief create a new tag. */
//    static void add_tag ( redis_ptr redis /** @param redis redis database pointer. */,
//                          const NodeType::Enum& type /** @param type the content module for the tag */,
//                          const std::string& tag /** @param tag name of the tag */,
//                          const std::string& value /** @param value of the tag */,
//                          const std::string& node /** @param the node this tag belongs to */,
//                          float score /** @param score the score of the node index */ ) {

//        redis->command ( { redis::ZADD, make_key ( key::FS, NodeType::str ( type ), key::TAG, param::NAME ), std::to_string ( score ), tag } );
//        redis->command ( { redis::ZADD, make_key ( key::FS, NodeType::str ( type ), key::TAG, tag ), std::to_string ( score ),  value } );
//        redis->command ( { redis::ZADD, make_key ( key::FS, NodeType::str ( type ), key::TAG, value ), std::to_string ( score ), node } );
//    }

    /** @brief evaluate the lua script. */
    template< class... ARGS>
    static std::vector< std::string > eval (  redis_ptr redis /** @param redis the database pointer. */,
            const std::string& script,
            int argc,
            ARGS... argv ) {
        auto& _result = redis->commandSync<std::vector< std::string >> ( { "EVAL", script, std::to_string ( argc ), argv... } );
        return _result.reply();
    }

// -----------------------------------------------------------------------------------------------------------
// --------------------------                  content relations                    --------------------------
// -----------------------------------------------------------------------------------------------------------

    /** @brief check if a node exists in the database. */
    static bool exists ( redis_ptr redis /** @param redis redox database pointer. */,
                         const std::string key  /** @param the node key. */ ) {
        redox::Command<int>& c = redis->commandSync<int> ( { redis::EXISTS, key } );
        return ( c.ok() && c.reply() );
    }

    /** @brief nodes by type. */
    static std::error_code nodes ( /** redis database pointer. */  redis_ptr redis,
            /** node type score. */         const long type,
            /** the node key. */            const std::string& key,
            /** result call back */         std::function< std::error_code ( redis_ptr, const std::string& ) > fn ) {

        redox::Command< std::vector< std::string > >& _c =
            redis->commandSync< std::vector< std::string > > ( { "ZRANGEBYSCORE", key,
                    std::to_string ( type ), std::to_string ( type ) } );

        if ( _c.ok() ) {
            for ( const std::string& __c : _c.reply() ) {
                auto _errc = fn ( redis, __c );

                if ( !!_errc )
                { return _errc; }
            }
        }
    }


private:
    static void __iterate_key (
        std::string& key,
        const std::string& value ) {
        key.append ( ":" );
        key.append ( value );
    }
    template< class... ARGS >
    static void __iterate_key (
        std::string& key,
        const std::string& value,
        ARGS... args ) {
        key.append ( ":" );
        key.append ( value );
        __iterate_key ( key, args... );
    }
    FRIEND_TEST ( DatastoreTest, to_map );
    static std::map< std::string, std::string > to_map ( command_t in ) {
        std::map< std::string, std::string > _map;
        assert ( in.size() % 2 == 0 );

        for ( size_t i = 0; i < in.size(); ++++i ) {
            _map[ in.at ( i ) ] = in.at ( i + 1 );
        }

        return _map;
    }
};//struct data
#endif // DATASTORE_H
