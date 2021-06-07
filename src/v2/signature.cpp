#include <signature.hpp>
#include <cassert>

namespace signature2 {

byte_t * as_bytes( void * memory ) noexcept
{
    return reinterpret_cast< byte_t * >( memory );
}

const byte_t * as_bytes( const void * memory ) noexcept
{
    return reinterpret_cast< const byte_t * >( memory );
}

block_t::block_t( block_id_t block_id, std::size_t offset, std::size_t size, std::size_t chunk_size ) noexcept
    : block_id( block_id )
      , offset( offset )
      , size( size )
      , chunk_size( chunk_size )
{

}

std::size_t block_t::chunks_count() const noexcept
{
    return size / chunk_size;
}

std::size_t block_t::tail_size() const noexcept
{
    return size % chunk_size;
}

segment_t::segment_t( block_id_t block_id,
                      std::size_t chunk_size,
                      std::size_t chunk_count,
                      std::size_t tail_size,
                      chunk_id_t first_chunk_id,
                      memory_t && memory ) noexcept
    : block_id( block_id )
      , chunk_size( chunk_size )
      , chunk_count( chunk_count )
      , tail_size( tail_size )
      , first_chunk_id( first_chunk_id )
      , memory( std::move( memory ) )
{

}

hashed_segment_t::hashed_segment_t( block_id_t block_id,
                                    chunk_id_t first_chunk_id,
                                    std::size_t chunk_count,
                                    std::size_t size,
                                    hashed_segment_t::memory_t && memory ) noexcept
    : block_id( block_id )
      , first_chunk_id( first_chunk_id )
      , chunk_count( chunk_count )
      , size( size )
      , memory( std::move( memory ) )
{

}
}