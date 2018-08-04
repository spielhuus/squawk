#include "importalbum.h"

#include "../common/constants.h"

ImportAlbum::ImportAlbum() {}

void ImportAlbum::import_audio_log( const data::redis_ptr redox, const std::string& key ) {
    auto _files = redox.command( {  } );



    //get child nodes
    redox::Command< std::vector< std::string > >& _c =
        redox_->commandSync< std::vector< std::string > > ( {"ZRANGEBYSCORE", make_key( "fs", kex, "list" ),
                                                             std::to_string( FILE_TYPE::logfile ),
                                                             std::to_string( FILE_TYPE::logfile ) } );

    if ( _c.ok() ) {
        for ( const std::string& __c : _c.reply() ) {
            const std::string _param =
            }



    std::ifstream _if ( argv[2] + __logfile );
    auto _toc = discid::parse_logfile ( _if );

    if ( !_toc.empty() ) {
        std::cout << _toc << std::endl;
        std::cout << "LOG: ";
        get_uri ( argv[1], _toc );
    }
}
