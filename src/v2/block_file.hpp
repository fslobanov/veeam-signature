#pragma once

#include <signature.hpp>
#include <boost/filesystem.hpp>

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
    
    const boost::filesystem::path & get_path() const noexcept;
    std::size_t get_size() const noexcept;

private:
    const boost::filesystem::path path;
    std::size_t file_size;
    
    struct entry_t final
    {
        explicit entry_t( block_t && block ) noexcept;
        alignas( g_cache_line_size ) block_t block;
    };
    std::vector< entry_t > storage;
};

}


