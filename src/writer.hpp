#pragma once

#include <hasher.hpp>

namespace boost {
namespace filesystem {
class path;
}
}

namespace signature {

class writer_t final
{
public:
    void operator ()( const boost::filesystem::path & path, const hasher_t::signature_t & signature ) noexcept( false );
};

}




