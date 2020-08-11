/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CompressionUtils.hpp"

#include <fstream>
#include <algorithm>
#include <cassert>

#include <zlib.h>

namespace FreeHeroes::Conversion {

int zlibUncompressFromBuffer(const char* sourceData, size_t remainSize, std::ostream &dest)
{
    const size_t CHUNK = 16384;
    int ret;
    unsigned have;
    z_stream strm;
    //unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = std::min(remainSize, size_t(CHUNK));
        remainSize -= strm.avail_in;

        if (strm.avail_in == 0)
            break;
        strm.next_in = (decltype(strm.next_in))sourceData;
        sourceData += strm.avail_in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
                [[fallthrough]];
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            dest.write(reinterpret_cast<const char*>(out), have);
            if ( dest.fail() ) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

}
