/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "Compression.hpp"
#ifdef USE_ZLIB
#include <zlib.h>
#endif
#ifdef USE_ZSTD
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#endif

#include <fstream>
#include <sstream>
#include <cassert>

namespace FreeHeroes::Core {

namespace {
const size_t CHUNK = 16384;

#ifdef USE_ZLIB
/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(std::istream& source, std::vector<uint8_t>& dest, int level)
{
    int           ret, flush;
    unsigned      have;
    z_stream      strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit2(&strm, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);

    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        source.read((char*) in, CHUNK);
        strm.avail_in = static_cast<uInt>(source.gcount());
        if (source.fail() && !source.eof()) {
            (void) deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush        = source.eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out  = out;
            ret            = deflate(&strm, flush); /* no bad return value */
            assert(ret != Z_STREAM_ERROR);          /* state not clobbered */
            have = CHUNK - strm.avail_out;
            dest.insert(dest.end(), out, out + have);
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0); /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END); /* stream will be complete */

    /* clean up and return */
    (void) deflateEnd(&strm);
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(const std::vector<uint8_t>& source, std::ostream& dest)
{
    int      ret;
    unsigned have;
    z_stream strm;
    //unsigned char in[CHUNK];
    unsigned char  out[CHUNK];
    size_t         remainSize = source.size();
    const uint8_t* sourceData = source.data();

    /* allocate inflate state */
    strm.zalloc   = Z_NULL;
    strm.zfree    = Z_NULL;
    strm.opaque   = Z_NULL;
    strm.avail_in = 0;
    strm.next_in  = Z_NULL;

    int wbits = 16 + MAX_WBITS;

    ret = inflateInit2(&strm, wbits);

    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = std::min(remainSize, size_t(CHUNK)); //fread(in, 1, CHUNK, source);
        remainSize -= strm.avail_in;

        if (strm.avail_in == 0)
            break;
        strm.next_in = (decltype(strm.next_in)) sourceData;
        sourceData += strm.avail_in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out  = out;
            ret            = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR; /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void) inflateEnd(&strm);
                    return ret;
            }
            have = CHUNK - strm.avail_out;
            dest.write(reinterpret_cast<const char*>(out), have);
            if (dest.fail()) {
                (void) inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void) inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

#endif

struct ByteArrayHolderBufWriter : std::basic_streambuf<char, typename std::char_traits<char>> {
    using base_type   = std::basic_streambuf<char, typename std::char_traits<char>>;
    using int_type    = typename base_type::int_type;
    using traits_type = typename base_type::traits_type;

    ByteArrayHolderBufWriter(ByteArrayHolder& holder)
        : m_holder(holder)
    {}

    int_type overflow(int_type ch) override
    {
        if (traits_type::eq_int_type(ch, traits_type::eof()))
            return traits_type::eof();
        m_holder.ref().push_back(traits_type::to_char_type(ch));
        return ch;
    }

protected:
    ByteArrayHolder& m_holder;
};

struct ByteArrayHolderBufReader : std::streambuf {
    ByteArrayHolderBufReader(const ByteArrayHolder& data)
    {
        this->setg((char*) data.data(), (char*) data.data(), (char*) data.data() + data.size());
    }
};

}

void uncompressDataBuffer(const ByteArrayHolder& input, ByteArrayHolder& output, CompressionInfo compressionInfo)
{
    if (false) {
    }
#ifdef USE_ZLIB
    else if (compressionInfo.m_type == CompressionType::Gzip) {
        ByteArrayHolderBufWriter outBuffer(output);
        std::ostream             outBufferStream(&outBuffer);
        auto                     infResult = inf(input.ref(), outBufferStream);
        if (infResult != Z_OK)
            throw std::runtime_error("Gzip inflate failed:" + std::to_string(infResult));
    }
#endif
#ifdef USE_ZSTD
    else if (compressionInfo.m_type == CompressionType::ZStd) {
        //size_t cSize;
        //void* const cBuff = loadFile_orDie(fname, &cSize);
        unsigned long long const rSize = ZSTD_findDecompressedSize(input.data(), input.size());
        if (rSize == ZSTD_CONTENTSIZE_ERROR)
            throw std::runtime_error("Data was not compressed by zstd.");
        else if (rSize == ZSTD_CONTENTSIZE_UNKNOWN)
            throw std::runtime_error("Original size unknown. Use streaming decompression instead.");

        output.resize(rSize);

        size_t const dSize = ZSTD_decompress(output.data(), rSize, input.data(), input.size());

        if (dSize != rSize)
            throw std::runtime_error("ZStd decompress failed:" + std::string(ZSTD_getErrorName(dSize)));
    }
#endif
    else if (compressionInfo.m_type == CompressionType::None) {
        output = input;
    } else {
        throw std::runtime_error("Unsupported compression type:" + std::to_string(static_cast<int>(compressionInfo.m_type)));
    }
}

void compressDataBuffer(const ByteArrayHolder& input, ByteArrayHolder& output, CompressionInfo compressionInfo)
{
    if (false) {
    }
#ifdef USE_ZLIB
    else if (compressionInfo.m_type == CompressionType::Gzip) {
        ByteArrayHolderBufReader inBuffer(input);
        std::istream             inBufferStream(&inBuffer);
        auto                     result = def(inBufferStream, output.ref(), compressionInfo.m_level);
        if (result != Z_OK)
            throw std::runtime_error("Gzip deflate failed:" + std::to_string(result));
    }
#endif
#ifdef USE_ZSTD
    else if (compressionInfo.m_type == CompressionType::ZStd) {
        size_t const cBuffSize = ZSTD_compressBound(input.size());
        output.resize(cBuffSize);

        size_t const cSize = ZSTD_compress(output.data(), cBuffSize, input.data(), input.size(), compressionInfo.m_level);
        if (ZSTD_isError(cSize))
            throw std::runtime_error("ZStd compress failed:" + std::string(ZSTD_getErrorName(cSize)));
        output.resize(cSize);
    }
#endif
    else if (compressionInfo.m_type == CompressionType::None) {
        output = input;
    } else {
        throw std::runtime_error("Unsupported compression type:" + std::to_string(static_cast<int>(compressionInfo.m_type)));
    }
}

}
