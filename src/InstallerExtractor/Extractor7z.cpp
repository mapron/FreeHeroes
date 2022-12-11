/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Extractor7z.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <cassert>

#include "7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zFile.h"

namespace {

BOOL DirectoryExists(LPCTSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

struct CBufferInStream {
    ISeekInStream vt;
    char*         data;
    size_t        size;
    size_t        pos = 0;
};

SRes BufferInStream_Read(const ISeekInStream* pp, void* buf, size_t* size)
{
    const size_t originalSize = *size;
    if (originalSize == 0)
        return SZ_OK;

    CBufferInStream* p = CONTAINER_FROM_VTBL(pp, CBufferInStream, vt);

    const size_t remainSize = p->size - p->pos;
    auto         readSize   = std::min(originalSize, remainSize);
    *size                   = readSize;

    assert(*size == originalSize);

    memcpy(buf, p->data + p->pos, readSize);
    p->pos += readSize;
    return SZ_OK;
}

SRes BufferInStream_Seek(const ISeekInStream* pp, Int64* pos, ESzSeek origin)
{
    CBufferInStream* p = CONTAINER_FROM_VTBL(pp, CBufferInStream, vt);
    if (origin == SZ_SEEK_SET)
        p->pos = 0 + *pos;
    else if (origin == SZ_SEEK_CUR)
        p->pos = p->pos + *pos;
    else if (origin == SZ_SEEK_END)
        p->pos = p->size + *pos;
    *pos = p->pos;
    return SZ_OK;
}

} // namespace

class Extractor7zImpl {
public:
    CBufferInStream bufferStream{};
    CLookToRead2    lookStream{};

    CSzArEx  db{};
    ISzAlloc allocImp{};
    ISzAlloc allocTempImp{};
    Byte     labuffer[0x40000];

    std::vector<std::wstring> archiveFilenames;
};

Extractor7z::Extractor7z(void* data, size_t size)
    : m_impl(new Extractor7zImpl())
{
    m_impl->allocImp.Alloc = SzAlloc;
    m_impl->allocImp.Free  = SzFree;

    m_impl->allocTempImp.Alloc = SzAllocTemp;
    m_impl->allocTempImp.Free  = SzFreeTemp;

    m_impl->bufferStream.data = static_cast<char*>(data);
    m_impl->bufferStream.size = size;

    m_impl->bufferStream.vt.Read = BufferInStream_Read;
    m_impl->bufferStream.vt.Seek = BufferInStream_Seek;

    LookToRead2_CreateVTable(&m_impl->lookStream, False);

    m_impl->lookStream.realStream = &m_impl->bufferStream.vt;
    m_impl->lookStream.buf        = m_impl->labuffer;
    m_impl->lookStream.bufSize    = sizeof(m_impl->labuffer);
    LookToRead2_Init(&m_impl->lookStream);

    CrcGenerateTable();

    SzArEx_Init(&m_impl->db);
    auto res = SzArEx_Open(&m_impl->db, &m_impl->lookStream.vt, &m_impl->allocImp, &m_impl->allocTempImp);
    if (res != SZ_OK)
        throw std::runtime_error(
            "Failed read archive data, maybe file is corrupted");

    ExtractFilenames();
}

Extractor7z::~Extractor7z()
{
    SzArEx_Free(&m_impl->db, &m_impl->allocImp);
}

void Extractor7z::ExtractFiles(const std::wstring& destPath)
{
    UInt32 blockIndex    = 0xFFFFFFFF; /* it can have any value before first call (if
                                     outBuffer = 0) */
    Byte*  outBuffer     = 0;          /* it must be 0 before first call for each new archive. */
    size_t outBufferSize = 0;          /* it can have any value before first call (if outBuffer = 0) */

    for (UInt32 i = 0; i < m_impl->db.NumFiles; i++) {
        std::wstring fname = m_impl->archiveFilenames[i];
        if (fname.empty())
            continue;

        size_t offset           = 0;
        size_t outSizeProcessed = 0;
        auto   res              = SzArEx_Extract(&m_impl->db, &m_impl->lookStream.vt, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &m_impl->allocImp, &m_impl->allocTempImp);
        if (res != SZ_OK)
            throw std::runtime_error("Failed to extract file data!");

        CreateFolders(fname, destPath);

        CSzFile    outFile;
        const auto extractedPath = destPath + fname;
        if (OutFile_OpenW(&outFile, extractedPath.c_str()))
            throw std::runtime_error("Can not open destination file for write");

        size_t processedSize = outSizeProcessed;

        if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed) {
            ISzAlloc_Free(&m_impl->allocImp, outBuffer);
            throw std::runtime_error("Can not write output file");
        }

        if (File_Close(&outFile)) {
            ISzAlloc_Free(&m_impl->allocImp, outBuffer);
            throw std::runtime_error("Can not close output file");
        }
    }
}

void Extractor7z::CreateFolders(std::wstring        fname,
                                const std::wstring& destPath)
{
    for (size_t j = 0; fname[j] != 0; j++) {
        if (fname[j] == L'/') {
            fname[j]    = 0;
            auto subdir = (destPath + fname.c_str());
            if (!DirectoryExists(subdir.c_str()) && !CreateDirectory(subdir.c_str(), NULL))
                throw std::runtime_error("Failed to create directory");
            fname[j] = CHAR_PATH_SEPARATOR;
        }
    }
}

void Extractor7z::ExtractFilenames()
{
    m_impl->archiveFilenames.resize(m_impl->db.NumFiles);
    std::vector<UInt16> tempStr;
    for (UInt32 i = 0; i < m_impl->db.NumFiles; i++) {
        unsigned isDir = SzArEx_IsDir(&m_impl->db, i);
        if (isDir)
            continue;

        const size_t namelen = SzArEx_GetFileNameUtf16(&m_impl->db, i, NULL);
        tempStr.reserve(namelen + 1);

        SzArEx_GetFileNameUtf16(&m_impl->db, i, tempStr.data());

        const std::wstring fname((wchar_t*) tempStr.data());
        m_impl->archiveFilenames[i] = fname;
    }
}
