#pragma once

#include <boost/utility/string_view.hpp>
#include <boost/filesystem.hpp>

namespace signature {

struct arguments_t final
{
    arguments_t( std::string && input, std::string && output, std::size_t chunk_size ) noexcept( false );
    
    boost::filesystem::path input_path;
    boost::filesystem::path output_path;
    std::size_t chunk_size;
};

class parser_t final
{
public:
    arguments_t operator ()( signed argument_count, char ** arguments ) noexcept( false );

private:
    std::size_t parse_chunk_size( boost::string_view string ) noexcept( false );
    std::size_t parse_digits( boost::string_view string, std::size_t suffix_size ) noexcept( false );
    
};

}