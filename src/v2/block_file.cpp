#include <block_file.hpp>

#include <boost/filesystem.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace signature2 {

block_file_t::block_file_t( const boost::filesystem::path & path, std::size_t block_count, std::size_t chunk_size )
    : path( path )
    , file_size( 0 )
{
    boost::system::error_code ec;
    file_size = boost::filesystem::file_size( path, ec );
    if( ec )
    {
        throw std::runtime_error( fmt::format( "failed to retrieve input file '{}' size: {}", path.string(), ec.message() ) );
    }
    spdlog::info( "Input file '{}' size is: {} bytes", path.string(), file_size );
    
    std::size_t approximate_block_size = file_size / block_count;
    while( approximate_block_size < chunk_size && block_count > 1 )
    {
        --block_count;
        approximate_block_size = file_size / block_count;
    }
    
    spdlog::info(
        "File size - {}, approximate block size - {} bytes",
        file_size,
        approximate_block_size
    );
    
    std::size_t current_block_start = 0;
    storage.reserve( block_count );
    
    for( block_id_t index = 0; index < block_count - 1; ++index )
    {
        const auto chunks_in_block = 1 + ( approximate_block_size / chunk_size );
        const auto block_size = chunks_in_block * chunk_size;
        
        storage.emplace_back( block_t{ index, current_block_start, block_size, chunk_size } );
        current_block_start = current_block_start + ( block_size + 1 );
    }
    
    const std::size_t tail_block_size = file_size - current_block_start;
    storage.emplace_back( block_t{ block_count - 1, current_block_start, tail_block_size, chunk_size } );
    
    for( const auto & entry : storage )
    {
        spdlog::info(
            "Block: id - {}, chunks - {}, start - {}, end - {}",
            entry.block.block_id,
            entry.block.chunks_count(),
            entry.block.offset,
            entry.block.offset + entry.block.size
        );
    }
    
    assert( size
            == ( storage.back().block.offset + storage.back().block.size )
            && "Memory end mismatch" );
}

std::size_t block_file_t::block_count() const noexcept
{
    return storage.size();
}

const block_t * block_file_t::get_block( block_id_t block_id ) const noexcept
{
    if( storage.size() > block_id )
    {
        return &storage[ block_id ].block;
    }
    return nullptr;
}

const boost::filesystem::path & block_file_t::get_path() const noexcept
{
    return path;
}

std::size_t block_file_t::get_size() const noexcept
{
    return file_size;
}

block_file_t::entry_t::entry_t( block_t && block ) noexcept
    : block( block )
{

}
}