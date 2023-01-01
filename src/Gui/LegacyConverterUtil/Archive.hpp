/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ByteOrderStream.hpp"
#include "FsUtils.hpp"

namespace FreeHeroes {

class Archive {
public:
    enum class BinaryFormat
    {
        Invalid,
        LOD,
        HDAT,
        SND,
        VID
    };

public:
    BinaryFormat m_format = BinaryFormat::Invalid;

    struct Record {
        std::string m_basename;
        std::string m_originalBasename;
        std::string m_extWithDot;
        std::string m_originalExtWithDot;
        bool        m_compressOnDisk        = false;
        bool        m_compressInArchive     = false;
        uint32_t    m_uncompressedSizeCache = 0; // only for m_compressInArchive==true

        // fields for satisfying round-trip tests.
        uint32_t             m_unknown1 = 0;
        std::vector<uint8_t> m_filenameGarbage;
        size_t               m_binaryOrder = 0;
        bool                 m_isPadding   = false;

        ByteArrayHolder m_buffer;

        std::string fullname() const { return m_basename + m_extWithDot; }
    };
    std::vector<Record> m_records;

    struct HdatRecord {
        std::string              m_basename;
        std::string              m_filenameAlt;
        std::vector<std::string> m_txtChapters;

        bool                  m_hasBlob = false;
        std::vector<uint8_t>  m_blob;
        std::vector<uint32_t> m_params;
    };
    std::vector<HdatRecord> m_hdatRecords;

    uint32_t             m_lodFormat = 0;
    std::vector<uint8_t> m_lodHeader;

public:
    Archive(std::ostream* logOutput = nullptr);

    void clear();

    void detectFormat(const Core::std_path& path, ByteOrderDataStreamReader& stream);

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;

    void saveToFolder(const Core::std_path& path, bool skipExisting) const;
    void loadFromFolder(const Core::std_path& path);

    void createFromFolder(const Core::std_path& path, const std::vector<std::string>& extensions);

    void convertToBinary();
    void convertFromBinary(bool uncompress);

private:
    void readBinaryHDAT(ByteOrderDataStreamReader& stream);
    void writeBinaryHDAT(ByteOrderDataStreamWriter& stream) const;

private:
    struct BinaryRecord {
        std::string          m_basename;
        std::string          m_extNoDot;
        std::vector<uint8_t> m_filenameGarbage; // garbage data after filename null terminator. if non-null.
        uint32_t             m_offset         = 0;
        uint32_t             m_fullSize       = 0;
        uint32_t             m_unknown1       = 0;
        uint32_t             m_compressedSize = 0;
        uint32_t             m_size           = 0; // if compressed - compressed. full otherwise.

        uint32_t m_offsetNext = 0;

        size_t m_headerOrder     = 0;
        size_t m_binaryDataOrder = 0;

        ByteArrayHolder m_buffer;

        void readBinaryLOD(ByteOrderDataStreamReader& stream);
        void writeBinaryLOD(ByteOrderDataStreamWriter& stream) const;

        void readBinarySND(ByteOrderDataStreamReader& stream);
        void writeBinarySND(ByteOrderDataStreamWriter& stream) const;

        void readBinaryVID(ByteOrderDataStreamReader& stream);
        void writeBinaryVID(ByteOrderDataStreamWriter& stream) const;
    };

    std::vector<BinaryRecord> m_binaryRecordsUnnamed;

    std::vector<BinaryRecord>  m_binaryRecords;
    std::vector<BinaryRecord*> m_binaryRecordsSortedByOffset;

    bool m_isBinary = false;

    std::ostream* m_logOutput = nullptr;
};

}
