#ifndef BYTEME_GZIP_FILE_READER_HPP
#define BYTEME_GZIP_FILE_READER_HPP

#include "zlib.h"
#include <stdexcept>
#include <vector>
#include <string>

/**
 * @file GzipFileReader.hpp
 *
 * @brief Read a Gzip-compressed file.
 */

namespace byteme {

/**
 * @brief Read uncompressed bytes from a Gzip-compressed file.
 *
 * This is basically a wrapper around Zlib's `gzFile` with correct closing and error checking.
 *
 * @tparam B Buffer type, usually a `char` or `unsigned char`.
 */
template<typename B = unsigned char>
class GzipFileReader {
private:
    /**
     * @cond
     */
    struct GZFile {
        GZFile(const char* path) : handle(gzopen(path, "rb")) {
            if (!handle) {
                throw std::runtime_error("failed to open file at '" + std::string(path) + "'");
            }
            return;
        }

        ~GZFile() {
            gzclose(handle);
            return;
        }

        // Delete the remaining constructors.
        GZFile(const GZFile&) = delete;
        GZFile(GZFile&&) = delete;
        GZFile& operator=(const GZFile&) = delete;
        GZFile& operator=(GZFile&&) = delete;

        gzFile handle;
    };
    /**
     * @endcond
     */

public:
    /**
     * @param path Path to the file.
     * @param buffer_size Size of the buffer to use for reading.
     */
    GzipFileReader(const char* path, size_t buffer_size = 65536) : gz(path), buffer_(buffer_size), read(0) {
        if constexpr(std::is_same<B, char>::value) {
            counter_buffer.resize(buffer_size);            
        }
        return;
    }

    /**
     * Read and decompress the next stretch of bytes from the input file.
     *
     * To read the entire file, this function should be called repeatedly until `false` is returned.
     * Note that `buffer()` and `available()` will still be valid on the last invocation (i.e., the one that returns `false`),
     * as some bytes may have been read before reaching the end of the file.
     *
     * @return Boolean indicating whether there are still bytes remaining in the file.
     */
    bool operator()() {
        read = gzread(gz.handle, buffer_.data(), buffer_.size());
        if constexpr(std::is_same<B, char>::value) {
            std::copy_n(buffer_.data(), read, counter_buffer.data());
        }

        if (read == 0) {
            if (!gzeof(gz.handle)) { 
                int dummy;
                throw std::runtime_error(gzerror(gz.handle, &dummy));
            }
            return false;
        } else {
            return true;
        }
    }

    /**
     * @return Pointer to the start of an array containing the decompressed bytes.
     * The number of available bytes is provided in `available()`.
     */
    const B* buffer() const  {
        if constexpr(std::is_same<B, char>::value) {
            return counter_buffer.data();
        } else {
            return buffer_.data();
        }
    }

    /**
     * @return Number of decompressed bytes available in the `buffer()`.
     */
    size_t available() const {
        return read;
    }

private:
    GZFile gz;
    std::vector<unsigned char> buffer_;
    std::vector<char> counter_buffer;
    size_t read;
};

}

#endif