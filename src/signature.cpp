#include <signature.hpp>
#include <cassert>

namespace signature {

byte_t * as_bytes( void * memory ) noexcept
{
    return reinterpret_cast< byte_t * >( memory );
}

const byte_t * as_bytes( const void * memory ) noexcept
{
    return reinterpret_cast< const byte_t * >( memory );
}

block_t::block_t( block_id_t block_id, const void * memory, std::size_t size, std::size_t chunk_size ) noexcept
    : block_id( block_id )
      , memory( memory )
      , size( size )
      , chunk_size( chunk_size )
{
    assert( block_t::memory );
}

std::size_t block_t::chunks_count() const noexcept
{
    return size / chunk_size;
}

std::size_t block_t::tail_size() const noexcept
{
    return size % chunk_size;
}

}