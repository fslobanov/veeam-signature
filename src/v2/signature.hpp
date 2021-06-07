#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace signature2 {

constexpr std::size_t g_cache_line_size = 64UL;

using block_id_t = std::size_t;
using chunk_id_t = std::size_t;
using byte_t = uint8_t;

const byte_t * as_bytes( const void * memory ) noexcept;
byte_t * as_bytes( void * memory ) noexcept;

struct block_t final
{
    block_t( block_id_t block_id, std::size_t offset, std::size_t size, std::size_t chunk_size ) noexcept;
    
    block_id_t block_id;
    
    std::size_t offset;
    std::size_t size;
    std::size_t chunk_size;
    
    std::size_t chunks_count() const noexcept;
    std::size_t tail_size() const noexcept;
};

struct segment_t final
{
public:
    using memory_t = std::unique_ptr< byte_t[] >;

public:
    segment_t( block_id_t block_id,
               std::size_t chunk_size,
               std::size_t chunk_count,
               std::size_t tail_size,
               chunk_id_t first_chunk_id,
               memory_t && memory ) noexcept;
    
    block_id_t block_id;
    
    std::size_t chunk_size;
    
    std::size_t chunk_count;
    std::size_t tail_size;
    
    chunk_id_t first_chunk_id;
    
    memory_t memory;
};

struct hashed_segment_t final
{
public:
    using memory_t = std::unique_ptr< byte_t[] >;

public:
    hashed_segment_t( block_id_t block_id,
                      chunk_id_t first_chunk_id,
                      std::size_t chunk_count,
                      std::size_t size,
                      memory_t && memory ) noexcept;
    
    block_id_t block_id;
    chunk_id_t first_chunk_id;
    
    std::size_t chunk_count;
    
    std::size_t size;
    memory_t memory;
};

}