/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "ebook.h"

#include "spdlog/spdlog.h"

namespace scanner {
EBook::EBook() {}

std::error_code EBook::import_file ( data::redis_ptr redis, const std::string& key, const std::string& path ) {
    SPDLOG_DEBUG ( spdlog::get ( SCANNER_LOGGER ), "import ebook: {}->{}", key, path );
    return std::error_code();
}
}//namespace scanner
