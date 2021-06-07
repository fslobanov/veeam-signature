#include <cassert>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/chrono.h>

#include <arguments.hpp>
#include <block_file.hpp>
#include <block_reader.hpp>
#include <hasher.hpp>
#include <writer.hpp>

signed main( int argument_count, char ** arguments ) noexcept
{
    const auto cpu_core_count = std::thread::hardware_concurrency();
    try
    {
        auto args = signature2::parser_t{}( argument_count, arguments );
        spdlog::info(
            "Generating file '{}' signature to '{}' with '{}' bytes chunks on {} cores ..",
            args.input_path.string(),
            args.output_path.string(),
            args.chunk_size,
            cpu_core_count
        );
        
        signature2::block_file_t block_file( args.input_path, cpu_core_count, args.chunk_size );
        spdlog::info( "Signature of '{}' will be split on {} block ..", args.input_path.string(), block_file.block_count() );
        
        boost::asio::io_context context;
        const auto stop_context = [ &context ]() noexcept
        {
            std::thread thread{
                [ &context ]() noexcept
                {
                    context.stop();
                }
            };
            thread.detach();
        };
        std::shared_ptr< std::exception_ptr > occurred_exception;
        
        signature2::writer_t writer{
            block_file, boost::asio::io_context::strand{ context }, args.output_path,
            [ &stop_context, &args, &context ]( signature2::writer_t::result_t && result ) noexcept( false )
            {
                if( auto success = boost::get< signature2::writer_t::success_t >( &result ) )
                {
                    ( void ) success;
                    stop_context();
                }
                else if( auto ec = boost::get< signature2::writer_t::error_t >( &result ) )
                {
                    boost::asio::post(
                        context,
                        [ ec = *ec, args ]() noexcept( false )
                        {
                            throw std::runtime_error( fmt::format(
                                "signature file'{}' write failed: {}",
                                args.output_path.string(),
                                ec ) );
                        }
                    );
                }
                else
                {
                    assert( false && "boost::static_visitor should be used" );
                }
            }
        };
        signature2::hasher_t hasher{
            context,
            [ &writer ]( signature2::hashed_segment_t && segment ) noexcept
            {
                writer( std::move( segment ) );
            }
        };
        
        std::vector< signature2::block_reader_t > block_readers;
        block_readers.reserve( block_file.block_count() );
        
        for( std::size_t core_index = 0; core_index < block_file.block_count(); ++core_index )
        {
            const auto block = block_file.get_block( core_index );
            block_readers.emplace_back(
                boost::asio::io_context::strand{ context }, args.input_path, *block,
                [ &hasher, &context ]( signature2::block_reader_t::result_t && result ) noexcept( false )
                {
                    if( auto segment = boost::get< signature2::segment_t >( &result ) )
                    {
                        hasher( std::move( *segment ) );
                    }
                    else if( auto ec = boost::get< signature2::block_reader_t::error_t >( &result ) )
                    {
                        boost::asio::post(
                            context,
                            [ ec = *ec ]() noexcept( false )
                            {
                                throw std::runtime_error( fmt::format( "failed to read file segment: {}", ec ) );
                            }
                        );
                    }
                    else
                    {
                        assert( false && "boost::static_visitor should be used" );
                    }
                }
            );
            block_readers.back()();
        }
        
        auto thread_worker = [ &context, &stop_context, &occurred_exception ]( std::size_t core_index ) noexcept
        {
            cpu_set_t cpu_mask;
            CPU_ZERO( &cpu_mask );
            CPU_SET( core_index, &cpu_mask );
            if( pthread_setaffinity_np( pthread_self(), sizeof( cpu_set_t ), &cpu_mask ) )
            {
                spdlog::warn( "Failed to set cpu affinity: {}", core_index );
            }
            
            const auto guard = boost::asio::make_work_guard( context );
            ( void ) guard;
            try
            {
                context.run();
            }
            catch ( const std::exception & exception )
            {
                std::atomic_store( &occurred_exception, std::make_shared< std::exception_ptr >( std::current_exception() ) );
                stop_context();
            }
            catch ( ... )
            {
                spdlog::error( "Unknown error occurred" );
                std::atomic_store( &occurred_exception, std::make_shared< std::exception_ptr >( std::current_exception() ) );
                stop_context();
            }
        };
        
        boost::thread_group pool;
        for( std::size_t core_index = 1; core_index < block_file.block_count(); ++core_index )
        {
            pool.create_thread(
                [ core_index, &thread_worker ]() noexcept
                {
                    thread_worker( core_index );
                }
            );
        }
        thread_worker( 0 );
        
        if( occurred_exception.get() )
        {
            std::rethrow_exception( *occurred_exception );
        }
        
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