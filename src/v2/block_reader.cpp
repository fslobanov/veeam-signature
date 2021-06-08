#include <cassert>
#include <block_reader.hpp>
#include <spdlog/spdlog.h>

namespace signature2 {

namespace {
constexpr auto g_segment_size_hint = 128UL * 1024UL * 1024UL;
}

block_reader_t::block_reader_t( boost::asio::io_context::strand && strand,
                                const boost::filesystem::path & path,
                                const block_t & block,
                                callback_t && callback ) noexcept( false )
    : strand( std::move( strand ) )
      , path( path )
      , block( block )
      , callback( std::move( callback ) )
      , segment_size(
        block.chunk_size > g_segment_size_hint
        ? block.chunk_size
        : ( g_segment_size_hint / block.chunk_size ) * block.chunk_size )
      , descriptor( 0 )
      , offset( 0 )
{
    
    //TODO use O_DIRECT, but read buffer alignment is required
    descriptor = open( path.c_str(), O_RDONLY /*| O_DIRECT */ );
    if( -1 == descriptor )
    {
        throw std::runtime_error( fmt::format(
            "failed to open file '{}' while reading block '{}': {}",
            path.string(),
            block.block_id,
            strerror( errno )
        ) );
    }
    
    if( const auto error = posix_fadvise( descriptor, block.offset, block.size, POSIX_FADV_SEQUENTIAL ) )
    {
        spdlog::error( "Failed to advise file reading: {}", strerror( error ) );
    }
}

block_reader_t::~block_reader_t() noexcept
{
    if( descriptor > 0 )
    {
        if( -1 == close( descriptor ) )
        {
            spdlog::error(
                "Failed to close file '{}' block '{}': {}",
                path.string(),
                block.block_id,
                std::strerror( errno )
            );
        }
    }
}

void block_reader_t::operator ()() noexcept
{
    boost::asio::post(
        strand,
        [ this ]() noexcept
        {
            do_read();
        }
    );
}

void block_reader_t::do_read() noexcept
{
    assert( offset < block.size );
    
    // on first attempt we read entire block in one read
    if( block.size <= g_segment_size_hint )
    {
        read_entire_block();
        return;
    }
    
    assert( offset <= block.size );
    const auto rest = block.size - offset;
    
    // reading full segment
    if( rest > segment_size )
    {
        read_segment();
        return;
    }
    
    //reading tail
    boost::asio::post(
        strand.context(),
        [ this, rest ]() noexcept
        {
            auto buffer = std::make_unique< byte_t[] >( rest );
            if( -1 == read( descriptor, buffer.get(), rest ) )
            {
                callback( std::strerror( errno ) );
                return;
            }
            
            segment_t segment{
                block.block_id,
                block.chunk_size,
                rest / block.chunk_size,
                rest % block.chunk_size,
                offset / block.chunk_size,
                std::move( buffer )
            };
            callback( std::move( segment ) );
        }
    );
}

void block_reader_t::read_entire_block() const noexcept
{
    // running read(2) somewhere in pool
    boost::asio::post(
        strand.context(),
        [ this ]() noexcept
        {
            auto buffer = std::make_unique< byte_t[] >( block.size );
            if( -1 == read( descriptor, buffer.get(), block.size ) )
            {
                callback( std::strerror( errno ) );
                return;
            }
            
            segment_t segment{
                block.block_id,
                block.chunk_size,
                block.chunks_count(),
                block.tail_size(),
                0,
                std::move( buffer )
            };
            callback( std::move( segment ) );
        }
    );
}

void block_reader_t::read_segment() noexcept
{
    const auto current_offset = offset;
    offset += segment_size;
    
    // running read(2) somewhere in pool
    boost::asio::post(
        strand.context(),
        [ this, current_offset ]() noexcept
        {
            auto buffer = std::make_unique< byte_t[] >( segment_size );
            if( -1 == read( descriptor, buffer.get(), segment_size ) )
            {
                callback( std::strerror( errno ) );
                return;
            }
            
            segment_t segment{
                block.block_id,
                block.chunk_size,
                segment_size / block.chunk_size,
                0,
                current_offset / block.chunk_size,
                std::move( buffer )
            };
            
            // read(2) completion callback executes in pool too
            callback( std::move( segment ) );
            
            // posting read next segment task to self, so reads will be queued on pool
            boost::asio::post(
                strand,
                [ this ]() noexcept
                {
                    do_read();
                }
            );
        }
    );
}

}




