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
    const auto result_size = ( chunks_count + ( tail_size ? 1 : 0 ) ) * sizeof( uint32_t );
    
    spdlog::info(
        "Block '{}' segment hashing started, first chunk - '{}', expected size - {} bytes",
        segment.block_id,
        segment.first_chunk_id,
        result_size );
    
    //TODO inplace memory hashing instead allocation
    auto hash = std::make_unique< byte_t[] >( result_size );
    
    std::size_t current_chunk = 0;
    for( std::size_t index = 0; index < chunks_count; ++index )
    {
        hash[ index * sizeof( uint32_t ) ] = compute_crc32( &segment.memory[ current_chunk ], segment.chunk_size );
        current_chunk += segment.chunk_size;
    }
    
    if( tail_size )
    {
        hash[ chunks_count * sizeof( uint32_t ) ] = compute_crc32( &segment.memory[ current_chunk ], tail_size );
    }
    
    spdlog::info( "Block '{}' hashing finished, first chunk - '{}'", segment.block_id, segment.first_chunk_id );
    //spdlog::info( "Block '{}' hash: {}", block.block_id, result );
    return { segment.block_id,
             segment.first_chunk_id,
             chunks_count,
             result_size,
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
        chunk = as_bytes( chunk ) + 1;
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

