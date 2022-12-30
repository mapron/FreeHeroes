/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpriteFile.hpp"

#include "PropertyTree.hpp"

#include "SpriteFileReflection.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"
#include "StringUtils.hpp"

#include <array>

namespace FreeHeroes {
using namespace Core;

namespace {

constexpr const std::array<uint8_t, 4> g_def32Signature{ { 0x44, 0x33, 0x32, 0x46 } }; // 'D32F'

struct SpriteDef32 {
    uint32_t format     = 0;
    int32_t  size       = 0;
    int32_t  fullWidth  = 0;
    int32_t  fullHeight = 0;
    int32_t  width      = 0;
    int32_t  height     = 0;
    int32_t  leftMargin = 0;
    int32_t  topMargin  = 0;
    uint32_t flags      = 0;
    uint32_t group      = 0;
};

struct SpriteDef {
    uint32_t blobSize   = 0;
    uint32_t format     = 0; /// format in which pixel data is stored 0-3
    uint32_t fullWidth  = 0;
    uint32_t fullHeight = 0;
    uint32_t width      = 0; /// width and height of pixel data, padding excluded
    uint32_t height     = 0;
    int32_t  leftMargin = 0;
    int32_t  topMargin  = 0;
};

}

void SpriteFile::detectFormat(const Core::std_path& path, ByteOrderDataStreamReader& stream)
{
    std::array<uint8_t, 4> signature;
    stream >> signature;

    std::string ext = path2string(path.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext == ".pcx") {
        m_format = BinaryFormat::PCX;
    } else if (ext == ".bmp") {
        m_format = BinaryFormat::BMP;
    } else if (ext == ".def" || ext == ".d32") {
        if (signature == g_def32Signature) {
            m_format = BinaryFormat::DEF32;
        } else {
            m_format = BinaryFormat::DEF;
        }
    } else {
        throw std::runtime_error("Failed to detect binary format for:" + path2string(path));
    }
}

void SpriteFile::readBinary(ByteOrderDataStreamReader& stream)
{
    if (m_format == BinaryFormat::DEF32) {
        std::array<uint8_t, 4> signature;
        stream >> signature;
        if (signature != g_def32Signature)
            throw std::runtime_error("DEF32 signature is not found");

        uint32_t version, count1;
        stream >> version >> count1 >> m_boundaryWidth >> m_boundaryHeight;
        assert(24 == count1);

        uint32_t u1, u2, u3;
        stream >> u1 >> u2 >> u3;
        assert(u1 == 1);
        assert(u2 == 8);
        assert(u3 == 1);

        uint32_t headerTotal, u5, frameCount, u7;
        stream >> headerTotal >> u5 >> frameCount >> u7;
        assert(u5 == 0);
        assert(headerTotal == 17 * frameCount + 16);
        assert(u7 == 4);

        m_groups.clear();
        m_groups.resize(1);
        Group& group = m_groups[0];

        group.m_frames.resize(frameCount);
        for (auto& frame : group.m_frames)
            stream.readStringWithGarbagePadding<13>(frame.m_bitmapFilename, frame.m_bitmapFilenamePad);

        {
            // offsets in file from the start
            std::map<uint32_t, size_t> offset2frameIndex;
            size_t                     i = 0;
            for (auto& frame : group.m_frames) {
                stream >> frame.m_originalOffset;
                if (offset2frameIndex.contains(frame.m_originalOffset)) {
                    frame.m_isDuplicate    = true;
                    frame.m_duplicateIndex = offset2frameIndex.at(frame.m_originalOffset);
                } else {
                    offset2frameIndex[frame.m_originalOffset] = i;
                }
                frame.m_index = i;
                i++;
            }
        }

        for (auto& frame : group.m_frames) {
            if (frame.m_isDuplicate) {
                continue;
            }
            const uint32_t streamOffset = stream.getBuffer().getOffsetRead();
            assert(frame.m_originalOffset == streamOffset); // otherwise frames are unsorted by offset

            SpriteDef32 def;
            stream
                >> def.format
                >> def.size
                >> def.fullWidth
                >> def.fullHeight
                >> def.width
                >> def.height
                >> def.leftMargin
                >> def.topMargin
                >> def.flags
                >> def.group; // group id ?
            const bool isEmpty = def.fullWidth == 0 || def.fullHeight == 0;
            if (isEmpty)
                assert(def.flags == 0);
            else
                assert(def.flags == 8);
            assert(def.group == 0);
            assert(def.size == def.width * def.height * 4);

            if (!isEmpty)
                assert(def.format == 32);

            if (!isEmpty) {
                BitmapFile bitmapFile;
                bitmapFile.m_compression     = BitmapFile::Compression::None;
                bitmapFile.m_pixFormat       = BitmapFile::PixFormat::BGRA;
                bitmapFile.m_width           = def.width;
                bitmapFile.m_height          = def.height;
                bitmapFile.m_inverseRowOrder = true;
                bitmapFile.readBinary(stream);

                const size_t bitmapIndex = m_bitmaps.size();
                m_bitmaps.push_back(std::move(bitmapFile));
                frame.m_bitmapIndex = bitmapIndex;
                frame.m_hasBitmap   = true;
            }

            frame.m_paddingLeft = def.leftMargin;
            frame.m_paddingTop  = def.topMargin;

            frame.m_bitmapWidth   = def.width;
            frame.m_bitmapHeight  = def.height;
            frame.m_bitmapOffsetX = 0;
            frame.m_bitmapOffsetY = 0;

            frame.m_boundaryWidth  = def.fullWidth;
            frame.m_boundaryHeight = def.fullHeight;
        }
        [[maybe_unused]] const auto remainingRead = stream.getBuffer().getRemainRead();
        assert(remainingRead == 0);
        return;
    }
    if (m_format == BinaryFormat::DEF) {
        uint32_t type = 0;
        stream >> type;
        assert(type >= 0x40 && type <= 0x49);
        m_defType = static_cast<DefType>(type);

        stream >> m_boundaryWidth >> m_boundaryHeight;

        const size_t totalGroups = stream.readSize();
        m_groups.resize(totalGroups);

        stream >> m_palette;

        std::map<uint32_t, size_t> offset2frameIndex;
        size_t                     frameIndex = 0;

        for (Group& group : m_groups) {
            stream >> group.m_groupId;
            const size_t totalFrames = stream.readSize();
            group.m_frames.resize(totalFrames);
            stream >> group.m_unk1 >> group.m_unk2;

            for (Frame& frame : group.m_frames) {
                stream.readStringWithGarbagePadding<13>(frame.m_bitmapFilename, frame.m_bitmapFilenamePad);
            }

            {
                for (auto& frame : group.m_frames) {
                    stream >> frame.m_originalOffset;
                    if (offset2frameIndex.contains(frame.m_originalOffset)) {
                        frame.m_isDuplicate    = true;
                        frame.m_duplicateIndex = offset2frameIndex.at(frame.m_originalOffset);
                    } else {
                        offset2frameIndex[frame.m_originalOffset] = frameIndex;
                    }
                    frame.m_index = frameIndex;
                    frameIndex++;
                }
            }
        }
        std::vector<Frame*> allFrames;
        {
            for (Group& group : m_groups) {
                for (Frame& frame : group.m_frames) {
                    if (frame.m_isDuplicate) {
                        continue;
                    }
                    allFrames.push_back(&frame);
                }
            }
            assert(!allFrames.empty());
        }

        std::sort(allFrames.begin(), allFrames.end(), [](Frame* l, Frame* r) {
            return l->m_originalOffset < r->m_originalOffset;
        });
        for (size_t i = 0; Frame * framep : allFrames) {
            framep->m_sortedIndex = i++;
        }
        for (Frame* framep : allFrames) {
            Frame&         frame        = *framep;
            const uint32_t streamOffset = stream.getBuffer().getOffsetRead();
            assert(frame.m_originalOffset == streamOffset); // otherwise frames are unsorted by offset

            SpriteDef def;
            stream
                >> def.blobSize
                >> def.format
                >> def.fullWidth
                >> def.fullHeight
                >> def.width
                >> def.height;

            //special case for some "old" format defs (SGTWMTA.DEF and SGTWMTB.DEF)
            if (def.format == 1 && def.width > def.fullWidth && def.height > def.fullHeight) {
                def.leftMargin = 0;
                def.topMargin  = 0;
                def.width      = def.fullWidth;
                def.height     = def.fullHeight;

            } else {
                stream
                    >> def.leftMargin
                    >> def.topMargin;
            }
            const bool isEmpty = def.fullWidth == 0 || def.fullHeight == 0;
            if (!isEmpty) {
                BitmapFile bitmapFile;
                if (def.format == 0)
                    bitmapFile.m_compression = BitmapFile::Compression::None;
                else if (def.format == 1)
                    bitmapFile.m_compression = BitmapFile::Compression::RLE1;
                else if (def.format == 2)
                    bitmapFile.m_compression = BitmapFile::Compression::RLE2;
                else if (def.format == 3)
                    bitmapFile.m_compression = BitmapFile::Compression::RLE3;
                else
                    throw std::runtime_error("Unknown RLE variant:" + std::to_string(def.format));
                bitmapFile.m_pixFormat       = BitmapFile::PixFormat::Gray;
                bitmapFile.m_width           = def.width;
                bitmapFile.m_height          = def.height;
                bitmapFile.m_inverseRowOrder = true;
                bitmapFile.m_rleData.m_size  = def.blobSize;
                bitmapFile.readBinary(stream);

                const size_t bitmapIndex = m_bitmaps.size();
                m_bitmaps.push_back(std::move(bitmapFile));
                frame.m_bitmapIndex = bitmapIndex;
                frame.m_hasBitmap   = true;
            }

            frame.m_paddingLeft = def.leftMargin;
            frame.m_paddingTop  = def.topMargin;

            frame.m_bitmapWidth   = def.width;
            frame.m_bitmapHeight  = def.height;
            frame.m_bitmapOffsetX = 0;
            frame.m_bitmapOffsetY = 0;

            frame.m_boundaryWidth  = def.fullWidth;
            frame.m_boundaryHeight = def.fullHeight;
        }
        [[maybe_unused]] const auto remainingRead = stream.getBuffer().getRemainRead();
        if (remainingRead > 0) {
            uint8_t a = 0, b = 0;
            stream >> a >> b;
            if (a == 0x0d && b == 0xa) {
                std::string s;
                s.resize(remainingRead - 2);
                stream.readBlock(s.data(), remainingRead - 2);
                m_tralilingData = splitLine(s, "\r\n");
            } else {
                assert(0);
            }
        }

        return;
    }
}

void SpriteFile::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    if (m_format == BinaryFormat::DEF32) {
        stream << g_def32Signature;

        if (m_groups.size() != 1)
            throw std::runtime_error("There must be exactly 1 frame group.");

        const Group& group = m_groups[0];

        uint32_t version, count1;
        version = 1;
        count1  = 24;
        stream << version << count1 << m_boundaryWidth << m_boundaryHeight;

        const uint32_t u1 = 1, u2 = 8, u3 = 1;
        stream << u1 << u2 << u3;

        uint32_t headerTotal, u5, frameCount, u7;
        frameCount  = group.m_frames.size();
        u5          = 0;
        headerTotal = 17 * frameCount + 16;
        u7          = 4;
        stream << headerTotal << u5 << frameCount << u7;

        for (auto& frame : group.m_frames)
            stream.writeStringWithGarbagePadding<13>(frame.m_bitmapFilename, frame.m_bitmapFilenamePad);

        std::vector<uint32_t> offsets(frameCount);
        const auto            offsetsOffset = stream.getBuffer().getOffsetWrite();
        stream.zeroPadding(4 * frameCount); // offsets in file

        for (const auto& frame : group.m_frames) {
            if (frame.m_isDuplicate) {
                assert(frame.m_duplicateIndex < frame.m_index);
                offsets[frame.m_index] = offsets[frame.m_duplicateIndex];
                continue;
            }

            const uint32_t currentOffset = stream.getBuffer().getOffsetWrite();
            offsets[frame.m_index]       = currentOffset;

            assert(frame.m_originalOffset == currentOffset);

            SpriteDef32 def;
            if (frame.m_hasBitmap) {
                def.fullWidth  = frame.m_boundaryWidth;
                def.fullHeight = frame.m_boundaryHeight;
                def.leftMargin = frame.m_paddingLeft;
                def.topMargin  = frame.m_paddingTop;
                def.width      = frame.m_bitmapWidth;
                def.height     = frame.m_bitmapHeight;
                def.size       = def.width * def.height * 4;
                def.flags      = 8;
                def.format     = 32;
            }
            stream
                << def.format
                << def.size
                << def.fullWidth
                << def.fullHeight
                << def.width
                << def.height
                << def.leftMargin
                << def.topMargin
                << def.flags
                << def.group;

            if (frame.m_hasBitmap) {
                const BitmapFile& bitmapFile = m_bitmaps[frame.m_bitmapIndex];
                stream << bitmapFile;
            }
        }
        stream.getBuffer().setOffsetWrite(offsetsOffset);
        for (auto& o : offsets)
            stream << o;
        return;
    }
    if (m_format == BinaryFormat::DEF) {
        stream << static_cast<uint32_t>(m_defType);
        stream << m_boundaryWidth << m_boundaryHeight;

        stream.writeSize(m_groups.size());
        stream << m_palette;

        std::vector<const Frame*> sortedFrames;

        for (const Group& group : m_groups) {
            stream << group.m_groupId;
            stream.writeSize(group.m_frames.size());
            stream << group.m_unk1 << group.m_unk2;

            for (const Frame& frame : group.m_frames) {
                stream.writeStringWithGarbagePadding<13>(frame.m_bitmapFilename, frame.m_bitmapFilenamePad);
            }
            for (const Frame& frame : group.m_frames) {
                stream << frame.m_originalOffset;
                if (!frame.m_isDuplicate)
                    sortedFrames.push_back(&frame);
            }
        }
        std::sort(sortedFrames.begin(), sortedFrames.end(), [](const Frame* l, const Frame* r) { return l->m_originalOffset < r->m_originalOffset; });

        for (const Frame* framep : sortedFrames) {
            const Frame& frame = *framep;
            if (frame.m_isDuplicate) {
                assert(frame.m_duplicateIndex < frame.m_index);
                //offsets[frame.m_index] = offsets[frame.m_duplicateIndex];
                continue;
            }

            const uint32_t currentOffset = stream.getBuffer().getOffsetWrite();
            //offsets[frame.m_index]       = currentOffset;

            assert(frame.m_originalOffset == currentOffset);

            SpriteDef def;
            if (frame.m_hasBitmap) {
                def.fullWidth                = frame.m_boundaryWidth;
                def.fullHeight               = frame.m_boundaryHeight;
                def.leftMargin               = frame.m_paddingLeft;
                def.topMargin                = frame.m_paddingTop;
                def.width                    = frame.m_bitmapWidth;
                def.height                   = frame.m_bitmapHeight;
                const BitmapFile& bitmapFile = m_bitmaps[frame.m_bitmapIndex];
                def.blobSize                 = bitmapFile.m_rleData.m_size;
                def.format                   = 0;
                if (bitmapFile.m_compression == BitmapFile::Compression::RLE1)
                    def.format = 1;
                if (bitmapFile.m_compression == BitmapFile::Compression::RLE2)
                    def.format = 2;
                if (bitmapFile.m_compression == BitmapFile::Compression::RLE3)
                    def.format = 3;
            }
            stream
                << def.blobSize
                << def.format
                << def.fullWidth
                << def.fullHeight
                << def.width
                << def.height
                << def.leftMargin
                << def.topMargin;

            if (frame.m_hasBitmap) {
                const BitmapFile& bitmapFile = m_bitmaps[frame.m_bitmapIndex];
                stream << bitmapFile;
            }
        }

        if (!m_tralilingData.empty()) {
            stream << uint8_t(0x0d) << uint8_t(0x0a);
            auto str = joinString(m_tralilingData, "\r\n");
            stream.writeBlock(str.c_str(), str.size());
        }

        return;
    }
}

void SpriteFile::toJson(PropertyTree& data) const
{
    Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void SpriteFile::fromJson(const PropertyTree& data)
{
    Reflection::PropertyTreeReader reader;
    reader.jsonToValue(data, *this);
}

void SpriteFile::bitmapsToJson(PropertyTree& data) const
{
    data["bitmaps"].convertToList();
    data["bitmaps"].getList().reserve(m_bitmaps.size());
    for (const BitmapFile& bitmap : m_bitmaps) {
        PropertyTree bdata;
        bitmap.toJson(bdata);
        data["bitmaps"].getList().push_back(std::move(bdata));
    }
}

void SpriteFile::bitmapsFromJson(const PropertyTree& data)
{
    if (data.contains("bitmaps")) {
        m_bitmaps.clear();
        const auto& blist = data["bitmaps"].getList();
        m_bitmaps.reserve(blist.size());
        for (const auto& bitmapJson : blist) {
            BitmapFile bitmapFile;
            bitmapFile.fromJson(bitmapJson);
            m_bitmaps.push_back(std::move(bitmapFile));
        }
    }
}

}
