#include <hasher.hpp>

#include <immintrin.h>
#include <pthread.h>

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <mapped_file.hpp>

#ifndef __SSE4_2__
#error "Sorry, you have a potato computer, go buy a new one"
#endif

namespace signature {

block_hasher_t::hash_t block_hasher_t::operator ()( const block_t & block ) const noexcept
{
    const auto chunks_count = block.chunks_count();
    const auto tail_size = block.tail_size();
    const auto expected_result_length = chunks_count + ( tail_size ? 1 : 0 );
    
    spdlog::info( "Block '{}' hashing started, expected size - {} bytes", block.block_id, expected_result_length * 4 );
    
    hash_t hash;
    hash.resize( expected_result_length );
    
    const void * current_chunk = block.memory;
    for( std::size_t index = 0; index < chunks_count; ++index )
    {
        hash[ index ] = compute_crc32( current_chunk, block.chunk_size );
        current_chunk = as_bytes( current_chunk ) + block.chunk_size;
    }
    
    if( tail_size )
    {
        hash[ chunks_count ] = compute_crc32( current_chunk, tail_size );
    }
    
    spdlog::info( "Block '{}' hashing finished ", block.block_id );
    //spdlog::info( "Block '{}' hash: {}", block.block_id, result );
    return hash;
}

block_hasher_t::crc32_t block_hasher_t::compute_crc32( const void * memory, std::size_t size ) const noexcept
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

hasher_t::signature_t hasher_t::operator ()( const mapped_file_t & mapped_file ) const noexcept
{
    boost::thread_group pool;
    
    signature_t signature;
    signature.resize( mapped_file.block_count() );
    
    for( std::size_t core_index = 0; core_index < mapped_file.block_count(); ++core_index )
    {
        auto block = mapped_file.get_block( core_index );
        assert( block && "Should exist" );
        
        pool.create_thread(
            [ core_index, block, &signature ]() noexcept
            {
                cpu_set_t cpu_mask;
                CPU_ZERO( &cpu_mask );
                CPU_SET( core_index, &cpu_mask );
                if( pthread_setaffinity_np( pthread_self(), sizeof( cpu_set_t ), &cpu_mask ) )
                {
                    spdlog::warn( "Failed to set cpu affinity: {}", core_index );
                }
                signature[ core_index ] = entry_t{ signature::block_hasher_t{}( *block ) };
            }
        );
    }
    pool.join_all();
    return signature;
}

}

