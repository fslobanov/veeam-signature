/*
 * \file signature.hpp
 * \copyright (C) 2021 Special Technological Center Ltd
 * \author : Lobanov F.S.
 * \date : 02.06.2021
 * \time : 17:20
 * \brief : 
 */

#pragma once

#include <cstdint>

namespace signature {

constexpr std::size_t g_cache_line_size = 64UL;

using block_id_t = std::size_t;
using byte_t = uint8_t;

const byte_t * as_bytes( const void * memory ) noexcept;
byte_t * as_bytes( void * memory ) noexcept;

struct block_t final
{
    block_t( block_id_t block_id, const void * memory, std::size_t size, std::size_t chunk_size ) noexcept;
    
    block_id_t block_id;
    
    const void * memory;
    std::size_t size;
    std::size_t chunk_size;
    
    std::size_t chunks_count() const noexcept;
    std::size_t tail_size() const noexcept;
};

}