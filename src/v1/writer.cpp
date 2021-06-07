#include <writer.hpp>
#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <hasher.hpp>

namespace signature {

namespace fs = boost::filesystem;

void writer_t::operator ()( const boost::filesystem::path & path, const hasher_t::signature_t & signature ) noexcept( false )
{
    boost::system::error_code ec;
    
    bool exists = false;
    try
    {
        exists = fs::exists( path, ec );
    }
    catch ( const std::exception & exception )
    {
        throw std::runtime_error( fmt::format( "failed to check output file '{}' existence: {}", path.string(), exception.what() ) );
    }
    
    if( exists )
    {
        spdlog::warn( "Signature file '{}' already exists, deleting", path.string() );
        fs::remove( path, ec );
        if( ec )
        {
            throw std::runtime_error( fmt::format( "failed to delete existing output file '{}': {}", path.string(), ec.message() ) );
        }
    }
    
    boost::filesystem::fstream stream;
    const auto mask = stream.exceptions() | std::ios::failbit | std::ios::badbit;
    stream.exceptions( mask );
    
    try
    {
        stream.open( path.c_str(), std::ios_base::out | std::ios_base::binary );
        
        for( const auto & block : signature )
        {
            stream.write( reinterpret_cast< const char * >( block.hash.data() ), block.hash.size() * sizeof( decltype( block.hash )::value_type ) );
        }
        
        stream.flush();
        stream.close();
    }
    catch ( const std::system_error & error )
    {
        throw std::runtime_error( fmt::format( "failed to write file '{}': {}", path.string(), error.what() ) );
    }
}

}

