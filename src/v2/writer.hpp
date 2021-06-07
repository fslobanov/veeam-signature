#pragma once

#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/variant.hpp>
#include <boost/container/flat_map.hpp>

#include <signature.hpp>
#include <block_file.hpp>

namespace signature2 {

class writer_t final
{
public:
    struct success_t final {};
    using error_t = std::string;
    using result_t = boost::variant< success_t, error_t >;
    using callback_t = std::function< void( result_t && result ) >;

public:
    writer_t( const block_file_t & block_file,
              boost::asio::io_context::strand && context,
              const boost::filesystem::path & path,
              callback_t && callback ) noexcept( false );

public:
    void operator ()( hashed_segment_t && segment ) noexcept;

private:
    boost::asio::io_context::strand strand;
    const callback_t callback;
    
    const std::size_t chunks_total;
    std::size_t chunks_count;
    boost::filesystem::fstream stream;
    
    struct key_t final
    {
        block_id_t block_id;
        chunk_id_t chunk_id;
        
        bool operator <( const key_t & rhs ) const noexcept;
    };
    boost::container::flat_map< key_t, hashed_segment_t > segments;
};

}