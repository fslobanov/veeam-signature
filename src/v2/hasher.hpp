#pragma once

#include <vector>
#include <functional>
#include <boost/asio.hpp>
#include <signature.hpp>

namespace signature2 {

class segment_hasher_t final
{
public:
    using crc32_t = uint32_t;
    using hash_t = hashed_segment_t;

public:
    hash_t operator ()( segment_t && segment ) const noexcept;

private:
    crc32_t compute_crc32( const void * memory, std::size_t size ) const noexcept;
};

class hasher_t final
{
public:
    using callback_t = std::function< void( hashed_segment_t && segment ) >;

public:
    explicit hasher_t( boost::asio::io_context & context, callback_t && callback ) noexcept;
    
    void operator ()( segment_t && segment ) noexcept;

private:
    boost::asio::io_context & context;
    const callback_t callback;
};

}




