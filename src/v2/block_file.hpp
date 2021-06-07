#pragma once

#include <signature.hpp>

namespace boost {
namespace filesystem {
class path;
}
}

namespace signature2 {

class block_file_t final
{
public:
    explicit block_file_t(
        const boost::filesystem::path & path,
        std::size_t block_count,
        std::size_t chunk_size ) noexcept( false );

public:
    std::size_t block_count() const noexcept;
    const block_t * get_block( block_id_t block_id ) const noexcept;

private:
    struct entry_t final
    {
        explicit entry_t( block_t && block ) noexcept;
        alignas( g_cache_line_size ) block_t block;
    };
    std::vector< entry_t > storage;
};

}


