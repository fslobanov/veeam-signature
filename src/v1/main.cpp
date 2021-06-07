#include <cassert>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/chrono.h>

#include <arguments.hpp>
#include <mapped_file.hpp>
#include <hasher.hpp>
#include <writer.hpp>

signed main( int argument_count, char ** arguments ) noexcept
{
    const auto cpu_core_count = std::thread::hardware_concurrency();
    try
    {
        auto args = signature::parser_t{}( argument_count, arguments );
        spdlog::info(
            "Generating file '{}' signature to '{}' with '{}' bytes chunks on {} cores ..",
            args.input_path.string(),
            args.output_path.string(),
            args.chunk_size,
            cpu_core_count
        );
        
        signature::mapped_file_t mapped_file( args.input_path, cpu_core_count, args.chunk_size );
        spdlog::info( "Signature of '{}' will be split on {} blocks ..", args.input_path.string(), mapped_file.block_count() );
        
        auto signature = signature::hasher_t{}( mapped_file );
        spdlog::info( "Generating signature of '{}' done ..", args.input_path.string() );
        
        signature::writer_t{}( args.output_path, signature );
        
        boost::system::error_code ignored;
        spdlog::info(
            "Signature file'{}' write successful, size is '{}' bytes ..",
            args.output_path.string(),
            boost::filesystem::file_size( args.output_path, ignored )
        );
    }
    catch ( const std::exception & exception )
    {
        spdlog::error( "Error occurred: {}", exception.what() );
        return EXIT_FAILURE;
    }
    catch ( ... )
    {
        spdlog::error( "Unknown error occurred" );
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}