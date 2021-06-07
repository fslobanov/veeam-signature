#pragma once

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

#include <signature.hpp>

namespace signature2 {

class block_reader_t final
{
public:
    using error_t = std::string;
    using result_t = boost::variant< segment_t, error_t >;
    using callback_t = std::function< void( result_t && ) >;

public:
    explicit block_reader_t(
        boost::asio::io_context::strand && strand,
        const boost::filesystem::path & path,
        const block_t & block,
        callback_t && callback ) noexcept( false );
    
    ~block_reader_t() noexcept;

public:
    void operator ()() noexcept;

private:
    boost::asio::io_context::strand strand;
    
    const boost::filesystem::path & path;
    const block_t & block;
    
    const callback_t callback;
    
    const std::size_t segment_size;
    int descriptor;
    std::size_t offset;

private:
    void do_read() noexcept;
    void read_entire_block() const noexcept;
    void read_segment() noexcept;
};

}



