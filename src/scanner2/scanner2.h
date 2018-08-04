#ifndef SCANNER2_H
#define SCANNER2_H

#include <memory>
#include <string>

#include "../common/datastore.h"

class Scanner2
{
public:
    Scanner2 ( const std::string& redis, const std::string& redis_port );
private:
    const data::redis_ptr redox_;
};

#endif // SCANNER2_H
