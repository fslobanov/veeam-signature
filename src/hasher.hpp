#pragma once

#include <vector>
#include <signature.hpp>

namespace signature {

class block_hasher_t final
{
public:
    using crc32_t = uint32_t;
    using hash_t = std::vector< crc32_t >;

public:
    hash_t operator ()( const block_t & block ) const noexcept;

private:
    crc32_t compute_crc32( const void * memory, std::size_t size ) const noexcept;
};

class mapped_file_t;

class hasher_t final
{
public:
    struct entry_t final
    {
        using hash_type = block_hasher_t::hash_t;
        alignas( g_cache_line_size ) block_hasher_t::hash_t hash;
    };
    using signature_t = std::vector< entry_t >;

public:
    signature_t operator ()( const mapped_file_t & mapped_file ) const noexcept;
};

}




