#include <hasher.hpp>
#include <immintrin.h>
#include <spdlog/spdlog.h>
#include <boost/asio.hpp>

#ifndef __SSE4_2__
#error "Sorry, you have a potato computer, go buy a new one"
#endif

namespace signature2 {

segment_hasher_t::hash_t segment_hasher_t::operator ()( segment_t && segment ) const noexcept
{
    const auto chunks_count = segment.chunk_count;
    const auto tail_size = segment.tail_size;
    const auto hash_size = ( chunks_count + ( tail_size ? 1 : 0 ) ) * sizeof( uint32_t );
    
    spdlog::info(
        "Block '{}' segment hashing started, first chunk - '{}', hash size - {} bytes",
        segment.block_id,
        segment.first_chunk_id,
        hash_size );
   
    auto hash = std::make_unique< byte_t[] >( hash_size );
    
    std::size_t current_chunk = 0;
    for( std::size_t index = 0; index < chunks_count; ++index )
    {
        const auto crc = compute_crc32( &segment.memory[ current_chunk ], segment.chunk_size );
        std::memcpy( &hash[ index * sizeof( uint32_t ) ], &crc, sizeof( crc ) );
        current_chunk += segment.chunk_size;
    }
    
    if( tail_size )
    {
        const auto crc = compute_crc32( &segment.memory[ current_chunk ], tail_size );
        std::memcpy( &hash[ chunks_count * sizeof( uint32_t ) ], &crc, sizeof( crc ) );
    }
    
    spdlog::info( "Block '{}' hashing finished, first chunk - '{}'", segment.block_id, segment.first_chunk_id );
    return { segment.block_id,
             segment.first_chunk_id,
             chunks_count,
             hash_size,
             std::move( hash ) };
}

segment_hasher_t::crc32_t segment_hasher_t::compute_crc32( const void * memory, std::size_t size ) const noexcept
{
    const std::size_t uint32s_count = size / sizeof( uint32_t );
    const std::size_t tail_size = size % sizeof( uint32_t );
    
    crc32_t crc = 0;
    auto chunk = static_cast< const uint8_t * >( memory );
    
    for( std::size_t index = 0; index < uint32s_count; ++index )
    {
        crc = _mm_crc32_u32( crc, *reinterpret_cast< const uint32_t * >( chunk ) );
        chunk = as_bytes( chunk ) + sizeof( uint32_t );
    }
    
    for( std::size_t index = 0; index < tail_size; ++index )
    {
        crc = _mm_crc32_u8( crc, *reinterpret_cast< const uint8_t * >( chunk ) );
    }
    
    return crc;
}

hasher_t::hasher_t( boost::asio::io_context & context, hasher_t::callback_t && callback ) noexcept
    : context( context )
      , callback( std::move( callback ) )
{

}

void hasher_t::operator ()( segment_t && segment ) noexcept
{
    boost::asio::post(
        context,
        [ this, segment = std::move( segment ) ]() mutable noexcept
        {
            auto hash = segment_hasher_t{}( std::move( segment ) );
            callback( std::move( hash ) );
        }
    );
}

}

