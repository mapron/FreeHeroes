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
#include <iostream>

#include <QPixmap>
#include <QPainter>

namespace FreeHeroes {
using namespace Core;

struct SpriteDef {
    uint32_t pixelBitSize = 0; /// HOTA only
    uint32_t blobSize     = 0;
    uint32_t rleCompress  = 0; /// SOD only. format in which pixel data is stored 0-3
    uint32_t fullWidth    = 0;
    uint32_t fullHeight   = 0;
    uint32_t width        = 0; /// width and height of pixel data, padding excluded
    uint32_t height       = 0;
    int32_t  leftMargin   = 0;
    int32_t  topMargin    = 0;

    uint32_t flags = 0; /// HOTA only
    int32_t  group = 0; /// HOTA only. not sure if it is group or what.
};

struct OffsetWriteTask {
    ptrdiff_t m_streamOffset = 0;
    uint32_t  m_frameOffset  = 0;
};
struct OffsetTracker {
    ByteOrderDataStreamWriter& m_stream;

    std::map<size_t, OffsetWriteTask> m_index;

    OffsetTracker(ByteOrderDataStreamWriter& stream)
        : m_stream(stream)
    {
    }
    void skipOffsetAndStore(size_t frameIndex)
    {
        auto current                       = m_stream.getBuffer().getOffsetWrite();
        m_index[frameIndex].m_streamOffset = current;
        m_stream.zeroPadding(4);
    }
    void updateFrameOffset(size_t frameIndex)
    {
        auto current                      = m_stream.getBuffer().getOffsetWrite();
        m_index[frameIndex].m_frameOffset = current;
    }
    void setFrameOffsetAsDuplicate(size_t frameIndex, size_t source)
    {
        m_index[frameIndex].m_frameOffset = m_index[source].m_frameOffset;
    }

    void writeOffsets()
    {
        for (const auto& [frameIndex, task] : m_index) {
            m_stream.getBuffer().setOffsetWrite(task.m_streamOffset);
            m_stream << task.m_frameOffset;
        }
    }
};

namespace {

constexpr const std::array<uint8_t, 4> g_def32Signature{ { 0x44, 0x33, 0x32, 0x46 } }; // 'D32F'

std_path makeSuffix(int index)
{
    std::string suffix = std::to_string(index);
    while (suffix.size() < 3)
        suffix = "0" + suffix;
    return "_" + suffix;
}

std_path makePngName(const std_path& baseName, int index)
{
    std_path name = baseName;
    if (index >= 0)
        name += makeSuffix(index);
    name += ".png";
    return name;
}

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
    auto readFrameData = [&stream, this](Frame& frame, const SpriteDef& def) {
        const bool isEmpty = def.fullWidth == 0 || def.fullHeight == 0;

        if (!isEmpty) {
            BitmapFile bitmapFile;
            if (def.rleCompress == 0)
                bitmapFile.m_compression = BitmapFile::Compression::None;
            else if (def.rleCompress == 1)
                bitmapFile.m_compression = BitmapFile::Compression::RLE1;
            else if (def.rleCompress == 2)
                bitmapFile.m_compression = BitmapFile::Compression::RLE2;
            else if (def.rleCompress == 3)
                bitmapFile.m_compression = BitmapFile::Compression::RLE3;
            else
                throw std::runtime_error("Unknown RLE variant:" + std::to_string(def.rleCompress));
            bitmapFile.m_compressionOriginal    = bitmapFile.m_compression;
            bitmapFile.m_pixFormat              = def.pixelBitSize == 32 ? BitmapFile::PixFormat::BGRA : BitmapFile::PixFormat::Gray;
            bitmapFile.m_width                  = def.width;
            bitmapFile.m_height                 = def.height;
            bitmapFile.m_inverseRowOrder        = false;
            bitmapFile.m_rleData.m_originalSize = bitmapFile.m_compression == BitmapFile::Compression::None ? 0 : def.blobSize;

            ByteArrayHolder blob;
            blob.resize(def.blobSize);
            stream.readBlock(blob.data(), blob.size());
            bitmapFile.readFromBlob(blob);

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

        frame.m_boundaryWidth  = def.fullWidth;
        frame.m_boundaryHeight = def.fullHeight;
    };

    std::map<uint32_t, size_t> offset2frameIndex;
    size_t                     frameHeaderIndex = 0;

    // ==========================   HEADER =====================================

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
            for (auto& frame : group.m_frames) {
                stream >> frame.m_originalOffset;
                if (offset2frameIndex.contains(frame.m_originalOffset)) {
                    frame.m_isDuplicate    = true;
                    frame.m_dupHeaderIndex = offset2frameIndex.at(frame.m_originalOffset);
                } else {
                    offset2frameIndex[frame.m_originalOffset] = frameHeaderIndex;
                }
                frame.m_headerIndex = frameHeaderIndex;
                frameHeaderIndex++;
            }
        }
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
                        frame.m_dupHeaderIndex = offset2frameIndex.at(frame.m_originalOffset);
                    } else {
                        offset2frameIndex[frame.m_originalOffset] = frameHeaderIndex;
                    }
                    frame.m_headerIndex = frameHeaderIndex;
                    frameHeaderIndex++;
                }
            }
        }
    }
    // ==========================   INDEXING =====================================

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
        framep->m_binaryOrder = i++;
    }

    // ==========================   FRAMES =====================================

    for (Frame* framep : allFrames) {
        Frame&                          frame        = *framep;
        [[maybe_unused]] const uint32_t streamOffset = stream.getBuffer().getOffsetRead();
        assert(frame.m_originalOffset == streamOffset);

        SpriteDef def;
        if (m_format == BinaryFormat::DEF32) {
            stream
                >> def.pixelBitSize
                >> def.blobSize
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
            assert(m_groups[0].m_groupId == 0 || m_groups[0].m_groupId == def.group); // assertion failure means it CAN have different group ids.
            m_groups[0].m_groupId = def.group;
            assert(def.blobSize == def.width * def.height * 4);

            if (!isEmpty)
                assert(def.pixelBitSize == 32);
        }
        if (m_format == BinaryFormat::DEF) {
            stream
                >> def.blobSize
                >> def.rleCompress
                >> def.fullWidth
                >> def.fullHeight
                >> def.width
                >> def.height
                >> def.leftMargin
                >> def.topMargin;

            //special case for some "old" format defs (SGTWMTA.DEF and SGTWMTB.DEF)
            if (def.rleCompress == 1 && def.width > def.fullWidth && def.height > def.fullHeight) {
                def.leftMargin = 0;
                def.topMargin  = 0;
                def.width      = def.fullWidth;
                def.height     = def.fullHeight;
                stream.getBuffer().setOffsetRead(stream.getBuffer().getOffsetRead() - 16);
                frame.m_shortHeaderFormat = true;
            }
        }

        readFrameData(frame, def);
    }

    // ====================== trailer ========================
    const auto remainingRead = stream.getBuffer().getRemainRead();
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
}

void SpriteFile::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto writeFrameData = [&stream, this](const Frame& frame) {
        ByteArrayHolder blob;
        SpriteDef       def;
        if (frame.m_hasBitmap) {
            def.fullWidth  = frame.m_boundaryWidth;
            def.fullHeight = frame.m_boundaryHeight;
            def.leftMargin = frame.m_paddingLeft;
            def.topMargin  = frame.m_paddingTop;
            def.width      = frame.m_bitmapWidth;
            def.height     = frame.m_bitmapHeight;
            def.group      = m_groups[0].m_groupId;

            const BitmapFile& bitmapFile = m_bitmaps[frame.m_bitmapIndex];
            bitmapFile.writeToBlob(blob);

            def.blobSize     = blob.size();
            def.flags        = 8;
            def.pixelBitSize = bitmapFile.m_pixFormat == BitmapFile::PixFormat::Gray ? 8 : 32;

            def.rleCompress = 0;
            if (bitmapFile.m_compression == BitmapFile::Compression::RLE1)
                def.rleCompress = 1;
            if (bitmapFile.m_compression == BitmapFile::Compression::RLE2)
                def.rleCompress = 2;
            if (bitmapFile.m_compression == BitmapFile::Compression::RLE3)
                def.rleCompress = 3;
        }

        if (m_format == BinaryFormat::DEF32) {
            stream
                << def.pixelBitSize
                << def.blobSize
                << def.fullWidth
                << def.fullHeight
                << def.width
                << def.height
                << def.leftMargin
                << def.topMargin
                << def.flags
                << def.group;
        } else {
            stream
                << def.blobSize
                << def.rleCompress
                << def.fullWidth
                << def.fullHeight;
            if (!frame.m_shortHeaderFormat)
                stream
                    << def.width
                    << def.height
                    << def.leftMargin
                    << def.topMargin;
        }

        if (frame.m_hasBitmap)
            stream.writeBlock(blob.data(), blob.size());
    };
    std::vector<const Frame*> headerFrames;
    std::vector<const Frame*> normalFrames;
    std::vector<const Frame*> dupFrames;

    OffsetTracker offsetTracker(stream);

    {
        [[maybe_unused]] size_t headerIndex = 0;
        for (const Group& group : m_groups) {
            for (const Frame& frame : group.m_frames) {
                headerFrames.push_back(&frame);
                if (!frame.m_isDuplicate)
                    normalFrames.push_back(&frame);
                else
                    dupFrames.push_back(&frame);
                assert(frame.m_headerIndex == headerIndex);
                headerIndex++;
            }
        }
        std::sort(normalFrames.begin(), normalFrames.end(), [](const Frame* l, const Frame* r) {
            return l->m_binaryOrder < r->m_binaryOrder;
        });
    }

    if (m_format == BinaryFormat::DEF32) {
        stream << g_def32Signature;

        if (m_groups.size() != 1)
            throw std::runtime_error("There must be exactly 1 frame group.");

        uint32_t version, count1;
        version = 1;
        count1  = 24;
        stream << version << count1 << m_boundaryWidth << m_boundaryHeight;

        const uint32_t u1 = 1, u2 = 8, u3 = 1;
        stream << u1 << u2 << u3;

        uint32_t headerTotal, u5, frameCount, u7;
        frameCount  = headerFrames.size();
        u5          = 0;
        headerTotal = 17 * frameCount + 16;
        u7          = 4;
        stream << headerTotal << u5 << frameCount << u7;

        for (const Frame* frame : headerFrames)
            stream.writeStringWithGarbagePadding<13>(frame->m_bitmapFilename, frame->m_bitmapFilenamePad);

        for (const Frame* frame : headerFrames) {
            offsetTracker.skipOffsetAndStore(frame->m_headerIndex);
        }
    }
    if (m_format == BinaryFormat::DEF) {
        stream << static_cast<uint32_t>(m_defType);
        stream << m_boundaryWidth << m_boundaryHeight;

        stream.writeSize(m_groups.size());
        stream << m_palette;

        for (const Group& group : m_groups) {
            stream << group.m_groupId;
            stream.writeSize(group.m_frames.size());
            stream << group.m_unk1 << group.m_unk2;

            for (const Frame& frame : group.m_frames)
                stream.writeStringWithGarbagePadding<13>(frame.m_bitmapFilename, frame.m_bitmapFilenamePad);

            for (const Frame& frame : group.m_frames)
                offsetTracker.skipOffsetAndStore(frame.m_headerIndex);
        }
    }
    for (const Frame* frame : normalFrames) {
        offsetTracker.updateFrameOffset(frame->m_headerIndex);
        writeFrameData(*frame);
    }
    for (const Frame* frame : dupFrames) {
        offsetTracker.setFrameOffsetAsDuplicate(frame->m_headerIndex, frame->m_dupHeaderIndex);
    }

    if (!m_tralilingData.empty()) {
        stream << uint8_t(0x0d) << uint8_t(0x0a);
        auto str = joinString(m_tralilingData, "\r\n");
        stream.writeBlock(str.c_str(), str.size());
    }

    offsetTracker.writeOffsets();
}

void SpriteFile::toJson(PropertyTree& data) const
{
    Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
    data["bitmaps"].convertToList();
    data["bitmaps"].getList().reserve(m_bitmaps.size());
    for (const BitmapFile& bitmap : m_bitmaps) {
        PropertyTree bdata;
        bitmap.toJson(bdata);
        data["bitmaps"].getList().push_back(std::move(bdata));
    }
}

void SpriteFile::fromJson(const PropertyTree& data)
{
    Reflection::PropertyTreeReader reader;
    reader.jsonToValue(data, *this);
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

void SpriteFile::saveBitmapsData(const Core::std_path& jsonFilePath) const
{
    if (m_bitmaps.empty() || m_embeddedBitmapData)
        return;

    const std_path folder   = jsonFilePath.parent_path();
    auto           baseName = jsonFilePath.filename().stem();
    if (baseName.has_extension())
        baseName = baseName.stem();

    const bool splitFiles = m_bitmaps.size() > 1;
    if (splitFiles) {
        const std_path subfolder = folder / baseName;
        for (int i = 0; const BitmapFile& bitmap : m_bitmaps) {
            const std_path path = subfolder / makePngName(baseName, i++);
            bitmap.savePixmapQt(path);
        }
    } else {
        const std_path path = folder / makePngName(baseName, -1);
        m_bitmaps[0].savePixmapQt(path);
    }
}

void SpriteFile::loadBitmapsData(const Core::std_path& jsonFilePath)
{
    if (m_bitmaps.empty() || m_embeddedBitmapData)
        return;

    const std_path folder   = jsonFilePath.parent_path();
    auto           baseName = jsonFilePath.filename().stem();
    if (baseName.has_extension())
        baseName = baseName.stem();

    const bool splitFiles = m_bitmaps.size() > 1;
    if (splitFiles) {
        const std_path subfolder = folder / baseName;
        for (int i = 0; BitmapFile & bitmap : m_bitmaps) {
            const std_path path = subfolder / makePngName(baseName, i++);
            bitmap.loadPixmapQt(path);
        }
    } else {
        const std_path path = folder / makePngName(baseName, -1);
        m_bitmaps[0].loadPixmapQt(path);
    }
}

void SpriteFile::uncompress()
{
    for (auto& bitmap : m_bitmaps)
        bitmap.uncompress();
}

void SpriteFile::compressToOriginal()
{
    for (auto& bitmap : m_bitmaps)
        bitmap.compressToOriginal();
}

void SpriteFile::unpackPalette()
{
    if (m_format != BinaryFormat::DEF)
        return;
    for (auto& bitmap : m_bitmaps)
        bitmap.unpackPalette(m_palette);
}

void SpriteFile::makePalette()
{
    BitmapFile::Palette palette = m_palette;
    for (auto& bitmap : m_bitmaps)
        bitmap.countPalette(palette);

    for (size_t i = 0; i <= 6; ++i) {
        palette.m_index[palette.m_table[i]] = i;
        if (palette.m_counter.contains(palette.m_table[i]))
            palette.m_counter.erase(palette.m_table[i]);
    }
    std::vector<std::pair<BitmapFile::Pixel, size_t>> counter;
    for (auto&& [pix, cnt] : palette.m_counter)
        counter.push_back({ pix, cnt });

    std::sort(counter.begin(), counter.end(), [](const auto& l, const auto& r) { return l.second > r.second; });
    if (counter.size() > 246) {
        std::cerr << "Warning: palette size (" << counter.size() << ") exceeds limit of (" << 246 << "), less popular colors will be set to 0 (transparent):\n";
        for (size_t i = 246; i < counter.size(); ++i) {
            auto p             = counter[i].first;
            palette.m_index[p] = 0;
            std::cerr << "RGBA " << p.toString() << " (" << counter[i].second << " total count) \n";
        }
        counter.resize(246);
    }
    for (size_t i = 0; i < counter.size(); ++i) {
        auto p                 = counter[i].first;
        palette.m_index[p]     = i + 8;
        palette.m_table[i + 8] = p;
    }

    for (auto& bitmap : m_bitmaps)
        bitmap.packPalette(palette);

    m_palette = palette;
}

void SpriteFile::setEmbeddedData(bool flag)
{
    if (m_embeddedBitmapData == flag)
        return;
    m_embeddedBitmapData = flag;

    if (m_embeddedBitmapData) {
        for (auto& bitmap : m_bitmaps)
            bitmap.fromPixmapQt();
        if (m_format == BinaryFormat::DEF) {
            makePalette();
        }
        compressToOriginal();
    } else {
        uncompress();
        unpackPalette();

        for (auto& bitmap : m_bitmaps)
            bitmap.toPixmapQt();
    }
}

void SpriteFile::mergeBitmaps()
{
    if (m_embeddedBitmapData)
        throw std::runtime_error("Only split bitmaps can be merged");

    assert(!m_bitmaps.empty());
    if (m_bitmaps.empty())
        return;

    std::map<size_t, int> bitmapIndex2group;

    struct GroupBitmaps {
        int m_maxHeight  = 0;
        int m_totalWidth = 0;
        int m_x          = 0;
    };
    int totalHeight = 0;
    int maxWidth    = 0;

    std::map<int, GroupBitmaps> groupRowData;
    for (Group& group : m_groups) {
        GroupBitmaps& g = groupRowData[group.m_groupId];
        for (const Frame& frame : group.m_frames) {
            if (!frame.m_hasBitmap)
                continue;
            g.m_maxHeight = std::max(g.m_maxHeight, frame.m_bitmapHeight);
            g.m_totalWidth += frame.m_bitmapWidth;
        }
        group.m_bitmapOffsetY = totalHeight;
        totalHeight += g.m_maxHeight;
        maxWidth = std::max(maxWidth, g.m_totalWidth);
    }

    QPixmap out(maxWidth, totalHeight);
    out.fill(Qt::transparent);

    for (Group& group : m_groups) {
        int x = 0;
        for (Frame& frame : group.m_frames) {
            if (!frame.m_hasBitmap)
                continue;
            QPixmap pix           = *m_bitmaps[frame.m_bitmapIndex].m_pixmapQt.get();
            frame.m_bitmapIndex   = 0;
            frame.m_bitmapOffsetX = x;
            x += frame.m_bitmapWidth;

            QPainter painter(&out);
            painter.drawPixmap(QPoint(frame.m_bitmapOffsetX, group.m_bitmapOffsetY), pix);
        }
    }

    m_bitmaps.clear();
    m_bitmaps.resize(1);
    m_bitmaps[0].m_pixmapQt = std::make_shared<QPixmap>(std::move(out));
}

}
