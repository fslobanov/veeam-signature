#include <arguments.hpp>

#include <cassert>

#include <boost/program_options.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace signature2 {

namespace opt = boost::program_options;
namespace fs = boost::filesystem;

namespace {

const std::vector< std::pair< boost::string_view, std::size_t > > g_multipliers = {
    { "kb",  1000 },
    { "kib", 1024UL },
    { "mb",  std::pow( 1000UL, 2 ) },
    { "mib", std::pow( 1024UL, 2 ) },
    { "gb",  std::pow( 1000UL, 3 ) },
    { "gib", std::pow( 1024UL, 3 ) }
};

constexpr std::size_t g_max_chunk_size = 1000UL * 1000UL * 1000UL * 8UL;
constexpr std::size_t g_min_chunk_size = 64000UL;

}

arguments_t::arguments_t( std::string && input, std::string && output, std::size_t chunk_size ) noexcept( false )
    : input_path( std::move( input ) )
      , output_path( std::move( output ) )
      , chunk_size( chunk_size )
{
    boost::system::error_code ec;
    
    input_path = fs::absolute( input, ec );
    if( ec )
    {
        throw std::runtime_error( fmt::format( "failed to convert input path to absolute: {}", input_path.string() ) );
    }
    
    output_path = fs::absolute( output, ec );
    if( ec )
    {
        throw std::runtime_error( fmt::format( "failed to convert output path to absolute: {}", input_path.string() ) );
    }
    
    const auto exists = fs::exists( input_path, ec );
    if( ec )
    {
        throw std::runtime_error( fmt::format( "failed to check input file '{}' existence: {}", input_path.string(), ec.message() ) );
    }
    
    if( !exists )
    {
        throw std::invalid_argument( fmt::format( "'input' file does not exists: {}", input_path.string() ) );
    }
    
    auto status = fs::status( input_path, ec );
    if( ec )
    {
        throw std::runtime_error( fmt::format( "failed to check input file '{}' status: {}", input_path.string(), ec.message() ) );
    }
    
    if( boost::filesystem::regular_file != status.type() )
    {
        throw std::invalid_argument( fmt::format( "'input' file is not regular: {}", input_path.string() ) );
    }
    
    /*if( status.permissions() & fs::perms::owner_read )
    {
    
    }*/
    
    if( chunk_size < g_min_chunk_size
        || chunk_size > g_max_chunk_size )
    {
        throw std::invalid_argument( fmt::format( "unsupported chunk size: {} bytes", chunk_size ) );
    }
}

arguments_t parser_t::operator ()( int argument_count, char ** arguments ) noexcept( false )
{
    std::string input_path;
    std::string output_path;
    std::string chunk_size;
    
    opt::options_description options_description;
    options_description.add_options()
                           ( "input,i", opt::value( &input_path ), "Source file" )
                           ( "output,o", opt::value( &output_path ), "Destination signature file" )
                           ( "chunk-size,c", opt::value( &chunk_size ), "Signature chunk size" );
    
    opt::variables_map variables;
    opt::store( opt::parse_command_line( argument_count, arguments, options_description ), variables );
    
    auto iterator = variables.find( "input" );
    if( variables.cend() == iterator )
    {
        throw std::invalid_argument( "'input' argument missing" );
    }
    input_path = iterator->second.as< std::string >();
    
    iterator = variables.find( "output" );
    if( variables.cend() == iterator )
    {
        throw std::invalid_argument( "'output' argument missing" );
    }
    output_path = iterator->second.as< std::string >();
    
    iterator = variables.find( "chunk-size" );
    
    if( variables.cend() != iterator )
    {
        chunk_size = iterator->second.as< std::string >();
        boost::algorithm::to_lower( chunk_size );
        
        return arguments_t{
            std::move( input_path ),
            std::move( output_path ),
            parse_chunk_size( chunk_size )
        };
    }
    
    return arguments_t{
        std::move( input_path ),
        std::move( output_path ),
        1000 * 1000
    };
}

std::size_t parser_t::parse_chunk_size( boost::string_view string ) noexcept( false )
{
    const auto found = std::find_if(
        g_multipliers.begin(), g_multipliers.end(),
        [ &string ]( const decltype( g_multipliers )::value_type & entry ) noexcept
        {
            return string.ends_with( entry.first );
        }
    );
    
    if( found != g_multipliers.end() )
    {
        return found->second * parse_digits( string, found->first.size() );
    }
    return parse_digits( string, 0 );
}

std::size_t parser_t::parse_digits( boost::string_view string, std::size_t suffix_size ) noexcept( false )
{
    try
    {
        boost::string_view digits{ string.data(), string.size() - suffix_size };
        return boost::lexical_cast< std::size_t >( digits );
    }
    catch ( const boost::bad_lexical_cast & exception )
    {
        throw std::invalid_argument( fmt::format( "invalid chunk size '{}': {} ", string.data(), exception.what() ) );
    }
}

}