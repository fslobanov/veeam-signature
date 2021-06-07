#include <writer.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace signature2 {

writer_t::writer_t( const block_file_t & block_file,
                    boost::asio::io_context::strand && strand,
                    const boost::filesystem::path & path,
                    callback_t && callback )
    
    : strand( std::move( strand ) )
      , callback( std::move( callback ) )
      , chunks_total(
        [ &block_file ]() noexcept -> std::size_t
        {
            std::size_t total = 0;
            for( std::size_t index = 0; index < block_file.block_count(); ++index )
            {
                auto block = block_file.get_block( index );
                total += block->chunks_count();
            }
            return total;
        }() )
      , chunks_count( 0 )
{
    // TODO provide segment count heuristics from readers to determine total segment count
    segments.reserve( 64 );
    
    boost::system::error_code ec;
    bool exists = false;
    try
    {
        exists = boost::filesystem::exists( path, ec );
    }
    catch ( const std::exception & exception )
    {
        throw std::runtime_error( fmt::format( "failed to check output file '{}' existence: {}", path.string(), exception.what() ) );
    }
    
    if( exists )
    {
        spdlog::warn( "Signature file '{}' already exists, deleting", path.string() );
        boost::filesystem::remove( path, ec );
        if( ec )
        {
            throw std::runtime_error( fmt::format( "failed to delete existing output file '{}': {}", path.string(), ec.message() ) );
        }
    }
    
    const auto mask = stream.exceptions() | std::ios::failbit | std::ios::badbit;
    stream.exceptions( mask );
    
    try
    {
        stream.open( path.c_str(), std::ios_base::out | std::ios_base::binary );
    }
    catch ( const std::system_error & error )
    {
        throw std::runtime_error( fmt::format( "failed to write file '{}': {}", path.string(), error.what() ) );
    }
}

void writer_t::operator ()( hashed_segment_t && segment ) noexcept
{
    boost::asio::post(
        strand,
        [ this, segment = std::move( segment ) ]() mutable noexcept
        {
            const key_t key{ segment.block_id, segment.first_chunk_id };
            const auto emplace = segments.emplace( key, std::move( segment ) );
            assert( emplace.second && "Should not exist" );
            
            chunks_count += emplace.first->second.chunk_count;
            if( chunks_count != chunks_total )
            {
                return;
            }
            
            try
            {
                for( const auto & entry : segments )
                {
                    const auto & segment = entry.second;
                    stream.write(
                        reinterpret_cast< const char * >( segment.memory.get() ),
                        segment.size );
                }
                
                stream.flush();
                stream.close();
                
                callback( success_t{} );
            }
            catch ( const std::exception & exception )
            {
                //stream.close();
                callback( error_t{ exception.what() } );
            }
        }
    );
}

bool writer_t::key_t::operator <( const writer_t::key_t & rhs ) const noexcept
{
    if( block_id < rhs.block_id )
    {
        return true;
    }
    
    if( rhs.block_id < block_id )
    {
        return false;
    }
    return chunk_id < rhs.chunk_id;
}

}

