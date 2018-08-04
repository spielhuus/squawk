#ifndef EBOOK_H
#define EBOOK_H

#include <string>
#include <system_error>

#include "../common/constants.h"
#include "../common/datastore.h"

namespace scanner {

class EBook {
public:
    static std::error_code import_file ( /** Redis database pointer. */  data::redis_ptr redis,
            /** key of the ebook. */        const std::string& key,
            /** path of the ebook. */       const std::string& path );

private:
    EBook();
};
}//namespace scanner
#endif // EBOOK_H
