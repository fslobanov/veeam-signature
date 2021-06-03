#include <mapped_file.hpp>

#include <fcntl.h>
#include <sys/mman.h>

#include <boost/filesystem.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace signature {

mapped_file_t::mapped_file_t( const boost::filesystem::path & path, std::size_t block_count, std::size_t chunk_size ) noexcept( false )
    : memory( nullptr )
      , bytes( 0 )
{
    boost::system::error_code ec;
    bytes = boost::filesystem::file_size( path, ec );
    if( ec )
    {
        throw std::runtime_error( fmt::format( "failed to retrieve input file '{}' size: {}", path.string(), ec.message() ) );
    }
    spdlog::info( "Input file '{}' size is: {} bytes", path.string(), bytes );
    
    auto descriptor = open( path.string().data(), O_RDONLY );
    if( -1 == descriptor )
    {
        throw std::runtime_error( fmt::format( "failed to open input file '{}'", path.string() ) );
    }
    memory = mmap( nullptr, bytes, PROT_READ, MAP_PRIVATE, descriptor, 0 );
    
    std::size_t approximate_block_size = bytes / block_count;
    while( approximate_block_size < chunk_size && block_count > 1 )
    {
        --block_count;
        approximate_block_size = bytes / block_count;
    }
    
    spdlog::info(
        "Memory start - {}, memory end - {}, approximate block size - {} bytes",
        fmt::ptr( memory ),
        fmt::ptr( as_bytes( memory ) + bytes ),
        approximate_block_size
    );
    
    const void * current_block_start = memory;
    storage.reserve( block_count );
    
    for( std::size_t index = 0; index < block_count - 1; ++index )
    {
        const auto chunks_in_block = 1 + ( approximate_block_size / chunk_size );
        const auto block_size = chunks_in_block * chunk_size;
        
        storage.emplace_back( block_t{ index, current_block_start, block_size, chunk_size } );
        current_block_start = as_bytes( current_block_start ) + block_size + 1;
    }
    
    const std::size_t tail_block_size = as_bytes( as_bytes( memory ) + bytes ) - as_bytes( current_block_start );
    storage.emplace_back( block_t{ block_count - 1, current_block_start, tail_block_size, chunk_size } );
    
    for( const auto & entry : storage )
    {
        spdlog::info(
            "Block: id - {}, chunks - {}, start - {}, end - {}",
            entry.block.block_id,
            entry.block.chunks_count(),
            fmt::ptr( entry.block.memory ),
            fmt::ptr( as_bytes( entry.block.memory ) + entry.block.size )
        );
    }
    
    assert( ( as_bytes( memory ) + bytes
              == as_bytes( storage.back().block.memory ) + storage.back().block.size )
            && "Memory end mismatch" );
}

mapped_file_t::~mapped_file_t() noexcept
{
    if( memory )
    {
        munmap( memory, bytes );
    }
}

std::size_t mapped_file_t::block_count() const noexcept
{
    return storage.size();
}

const block_t * mapped_file_t::get_block( block_id_t block_id ) const noexcept
{
    if( storage.size() > block_id )
    {
        return &storage[ block_id ].block;
    }
    return nullptr;
}

mapped_file_t::entry_t::entry_t( block_t && block ) noexcept
    : block( std::move( block ) )
{

}

}


