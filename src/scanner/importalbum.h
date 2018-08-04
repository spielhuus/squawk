#ifndef IMPORTALBUM_H
#define IMPORTALBUM_H

#include <string>

#include "../common/datastore.h"

class ImportAlbum {
public:

  static void import_audio_log( const data::redis_ptr redox_, const std::string& key );

private:
  ImportAlbum();
};

#endif // IMPORTALBUM_H
