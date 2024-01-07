/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpriteFile.hpp"

#include "MernelPlatform/PropertyTree.hpp"

#include "SpriteFileReflection.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"
#include "MernelPlatform/StringUtils.hpp"
#include "MernelPlatform/Logger.hpp"

#include "Sprites.hpp"

#include "Pixmap.hpp"
#include "Painter.hpp"

#include <array>
#include <iostream>

namespace FreeHeroes {
using namespace Mernel;

namespace {

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

struct ExtendedPaletteColor {
    size_t                        m_index;
    BitmapFile::Pixel             m_possibleColor; // for the record
    BitmapFile::Pixel             m_replacement;
    std::set<SpriteFile::DefType> m_defTypes;
    bool                          m_check = false; // check if palette contains required color
};
struct ExtendedPaletteColorConfig {
    std::vector<ExtendedPaletteColor> m_table;

    void extendPalette(const BitmapFile::Palette& original, BitmapFile::Palette& ext, SpriteFile::DefType type) const
    {
        ext.m_table.resize(8);
        for (size_t i = 0; i < 8; i++) {
            auto& pix = ext.m_table[i];
            pix       = original.m_table[i];

            for (const ExtendedPaletteColor& clr : m_table) {
                if (clr.m_index == i && clr.m_defTypes.contains(type)) {
                    if (clr.m_check && original.m_table[clr.m_index] != clr.m_possibleColor)
                        continue;
                    //std::cerr << "replace [" << i << "] " << pix.toString() << " -> " << clr.m_replacement.toString() << "\n";
                    pix = clr.m_replacement;
                }
            }
        }
    }
    void fillReverseIndex(BitmapFile::Palette& palette, SpriteFile::DefType type) const
    {
        for (const ExtendedPaletteColor& clr : m_table) {
            if (clr.m_defTypes.contains(type)) {
                if (clr.m_check && palette.m_table[clr.m_index] != clr.m_possibleColor)
                    continue;
                palette.m_index[clr.m_replacement] = clr.m_index;
                if (palette.m_counter.contains(clr.m_replacement))
                    palette.m_counter.erase(clr.m_replacement);
            }
        }
    }
};

const ExtendedPaletteColorConfig g_extendedColors{ {
    ExtendedPaletteColor{
        // cyan    => transparent    100%
        .m_index         = 0,
        .m_possibleColor = { 0x00, 0xFF, 0xFF, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x00 },
        .m_defTypes      = {
            SpriteFile::DefType::BattleSpells,
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::BattleSprite,
            SpriteFile::DefType::AdventureItem,
            SpriteFile::DefType::AdventureHero,
            SpriteFile::DefType::AdventureTerrain,
            SpriteFile::DefType::Cursor,
            SpriteFile::DefType::Interface,
            SpriteFile::DefType::SpriteFrame,
            SpriteFile::DefType::BattleHero,
        },
    },
    ExtendedPaletteColor{
        // cyan    =>  l. mag. => shadow border , 25%
        .m_index         = 1,
        .m_possibleColor = { 0xFF, 0x96, 0xFF, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x40 },
        .m_defTypes      = {
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::BattleSprite,
            SpriteFile::DefType::AdventureItem,
            SpriteFile::DefType::AdventureHero,
            SpriteFile::DefType::AdventureTerrain,
            SpriteFile::DefType::Interface,
            SpriteFile::DefType::SpriteFrame,
            SpriteFile::DefType::BattleHero,
        },
    },
    ExtendedPaletteColor{
        // mag 1   => ???           , 30%
        .m_index         = 2,
        .m_possibleColor = { 0xFF, 0x64, 0xFF, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x50 },
        .m_defTypes      = {
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::AdventureItem,
            SpriteFile::DefType::AdventureHero,
            SpriteFile::DefType::AdventureTerrain,
            SpriteFile::DefType::SpriteFrame,
        },
        .m_check = true,
    },
    ExtendedPaletteColor{
        // mag 2   => ???           , 40%
        .m_index         = 3,
        .m_possibleColor = { 0xFF, 0x32, 0xFF, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x60 },
        .m_defTypes      = {
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::AdventureItem,
            SpriteFile::DefType::AdventureHero,
            SpriteFile::DefType::AdventureTerrain,
            SpriteFile::DefType::SpriteFrame,
        },
        .m_check = true,
    },
    ExtendedPaletteColor{
        // magenta => shadow body   , 50%
        .m_index         = 4,
        .m_possibleColor = { 0xFF, 0x00, 0xFF, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x80 },
        .m_defTypes      = {
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::BattleSprite,
            SpriteFile::DefType::AdventureItem,
            SpriteFile::DefType::AdventureHero,
            SpriteFile::DefType::AdventureTerrain,
            SpriteFile::DefType::Interface,
            SpriteFile::DefType::SpriteFrame,
            SpriteFile::DefType::BattleHero,
        },
    },

    ExtendedPaletteColor{
        // yellow  => flag color / selection highlight. almost transparent
        .m_index         = 5,
        .m_possibleColor = { 0xFF, 0xFF, 0x00, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x01 },
        .m_defTypes      = {
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::BattleSprite,
            SpriteFile::DefType::AdventureItem,
            SpriteFile::DefType::AdventureHero,
            SpriteFile::DefType::SpriteFrame,
        },
        .m_check = true,
    },
    ExtendedPaletteColor{
        // d. mag. => shadow body   + selection 50% + 1.
        .m_index         = 6,
        .m_possibleColor = { 0xB4, 0x00, 0xFF, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x81 },
        .m_defTypes      = {
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::BattleSprite,
            SpriteFile::DefType::SpriteFrame,
        },
    },
    ExtendedPaletteColor{
        // green   => shadow border + selection 25% + 1.
        .m_index         = 7,
        .m_possibleColor = { 0x00, 0xFF, 0x00, 0xFF },
        .m_replacement   = { 0x00, 0x00, 0x00, 0x41 },
        .m_defTypes      = {
            SpriteFile::DefType::Sprite,
            SpriteFile::DefType::BattleSprite,
            SpriteFile::DefType::SpriteFrame,
        },
    },
} };

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

void postProcessSpriteGroupBattler(int groupIndex, Gui::Sprite::Group& seq, const PropertyTree& params)
{
    enum class BattleAnimation
    {
        Move         = 0,
        Nervous      = 1,
        StandStill   = 2,
        PainRanged   = 3,
        PainMelee    = 4,
        Death        = 5,
        Death2       = 6,
        Turning      = 7,
        MeleeUp      = 11,
        MeleeCenter  = 12,
        MeleeDown    = 13,
        RangedUp     = 14,
        RangedCenter = 15,
        RangedDown   = 16,
        MagicUp      = 17,
        MagicCenter  = 18,
        MagicDown    = 19,
        MoveStart    = 20,
        MoveFinish   = 21,
    };
    assert(groupIndex <= 21);
    const BattleAnimation type   = static_cast<BattleAnimation>(groupIndex);
    bool                  isWide = false;
    if (params.contains("wide")) {
        isWide = params["wide"].getScalar().toBool();
    }
    std::vector<int> relativeSpeeds;
    if (params.contains("speed")) {
        for (const auto& val : params.getMap().at("speed").getList())
            relativeSpeeds.push_back(val.getScalar().toInt());
    }
    std::vector<int> actionPoints;
    int              special = -1;
    if (params.contains("actionPoints")) {
        special = 4;
        for (const auto& val : params.getMap().at("actionPoints").getList())
            actionPoints.push_back(val.getScalar().toInt());
    }
    if (params.contains("special")) {
        special = params["special"].getScalar().toInt();
    }

    auto getAnimDuration = [&relativeSpeeds, &type](int frames) {
        int duration = 1000;
        switch (type) {
            case BattleAnimation::Move:
                duration = 500;
                break;
            case BattleAnimation::Nervous:
                duration = 100;
                break; // per frame
            case BattleAnimation::StandStill:
                duration = 100;
                break; // per frame
            case BattleAnimation::Turning:
                duration = 100;
                break; // per frame
            case BattleAnimation::MeleeUp:
            case BattleAnimation::MeleeCenter:
            case BattleAnimation::MeleeDown:
            case BattleAnimation::RangedUp:
            case BattleAnimation::RangedCenter:
            case BattleAnimation::RangedDown:
            case BattleAnimation::MagicUp:
            case BattleAnimation::MagicCenter:
            case BattleAnimation::MagicDown:
                duration = 500;
                break; // 400 just attack, 100 both attack and pain
            case BattleAnimation::PainMelee:
            case BattleAnimation::PainRanged:
                duration = 400;
                break;
            case BattleAnimation::MoveStart:
                duration = 100;
                break; // per frame
            case BattleAnimation::MoveFinish:
                duration = 100;
                break; // per frame
            case BattleAnimation::Death:
                duration = 600;
                break;
            default:
                break;
        }
        const bool needMultiplyByFrames = type == BattleAnimation::Turning
                                          || type == BattleAnimation::StandStill
                                          || type == BattleAnimation::Nervous
                                          || type == BattleAnimation::MoveStart
                                          || type == BattleAnimation::MoveFinish;
        if (needMultiplyByFrames)
            duration *= frames;
        int relativeSpeed = 100;
        if (type == BattleAnimation::Move
            || type == BattleAnimation::MoveStart
            || type == BattleAnimation::MoveFinish)
            relativeSpeed = relativeSpeeds[0];
        else if (type == BattleAnimation::StandStill)
            relativeSpeed = relativeSpeeds[1];
        else if (type == BattleAnimation::Nervous)
            relativeSpeed = relativeSpeeds[2];

        return duration * relativeSpeed / 100;
    };

    // [34, -71, 38, -61, 17, -53]   , 7

    seq.m_params.m_animationCycleDuration = getAnimDuration(seq.m_frames.size());
    if (!actionPoints.empty()) {
        if (type == BattleAnimation::RangedUp)
            seq.m_params.m_actionPoint = { actionPoints[0], actionPoints[1] };
        else if (type == BattleAnimation::RangedCenter)
            seq.m_params.m_actionPoint = { actionPoints[2], actionPoints[3] };
        else if (type == BattleAnimation::RangedDown)
            seq.m_params.m_actionPoint = { actionPoints[4], actionPoints[5] };

        if (!seq.m_params.m_actionPoint.isNull())
            seq.m_params.m_specialFrameIndex = special;
    }
    for (auto& frame : seq.m_frames) {
        if (isWide) {
            frame.m_padding += PixmapPoint(6, -10); // That's strange, but Battle sprites not perfectly centered. That's unconvenient.
        } else {
            frame.m_padding += PixmapPoint(30, -10);
        }
    }
}

void replaceColors(Pixmap& pix, const PixmapColor& src, const PixmapColor& dest)
{
    for (auto& p : pix.m_pixels) {
        if (p.m_color == src)
            p.m_color = dest;
    }
}

struct AnimationPaletteShift {
    int m_from   = 0;
    int m_length = 0;
};
struct AnimationPaletteConfig {
    std::vector<AnimationPaletteShift> m_shifts;
    int                                m_variantsCount = 0;

    void shiftPalette(BitmapFile::Palette& palette) const
    {
        auto palCopy = palette;
        for (const AnimationPaletteShift& shift : m_shifts) {
            for (int i = 0; i < shift.m_length; ++i)
                palCopy.m_table[shift.m_from + (i + 1) % shift.m_length] = palette.m_table[shift.m_from + i];
        }
        palette = std::move(palCopy);
    }
};

}

void SpriteFile::detectFormat(const Mernel::std_path& path, ByteOrderDataStreamReader& stream)
{
    std::array<uint8_t, 4> signature;
    stream >> signature;

    std::string ext = pathToLower(path.extension());
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
    if (m_format == BinaryFormat::PCX) {
        return readBinaryPCX(stream);
    }
    if (m_format == BinaryFormat::BMP) {
        assert(false);
        return;
    }
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
            bitmapFile.m_inverseRowOrder        = m_format == BinaryFormat::DEF32;
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
        assert((u1 == 1 && u2 == 8 && u3 == 1) || (u1 == 18 && u2 == 8 && u3 == 22) || (u1 == 16 && u2 == 8 && u3 == 22));

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
    bool skipRemainingReadCheck = false;
    for (Frame* framep : allFrames) {
        Frame&         frame        = *framep;
        const uint32_t streamOffset = stream.getBuffer().getOffsetRead();
        if (frame.m_originalOffset != streamOffset) {
            stream.getBuffer().setOffsetRead(frame.m_originalOffset);
            skipRemainingReadCheck = true;
        }

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
        } else if (m_format == BinaryFormat::DEF) {
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
            // special case for new buggy Hota 1.7 sprites like avlmhs02.def
            const auto remainingRead = stream.getBuffer().getRemainRead();
            if (def.blobSize > remainingRead) {
                if (def.blobSize - 32 == remainingRead) {
                    def.blobSize -= 32;
                }
            }
        } else {
            throw std::runtime_error("Unknown/unsupported format");
        }

        readFrameData(frame, def);
    }

    // ====================== trailer ========================
    const auto remainingRead = stream.getBuffer().getRemainRead();
    if (!skipRemainingReadCheck && remainingRead > 0) {
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

void SpriteFile::readBinaryPCX(Mernel::ByteOrderDataStreamReader& stream)
{
    BitmapFile bmp;
    uint32_t   fileSize = stream.getBuffer().getSize();

    uint32_t size = 0, width = 0, height = 0;
    stream >> size >> width >> height;

    if (size == 0x46323350 && width == 0 && height == 32) // 'P32F'
    {
        uint32_t u1 = 0, u2 = 0, u3 = 0, u4 = 0;
        stream >> size >> u1 >> u2 >> width >> height >> u3 >> u4;

        Pixmap tmp(width, height);
        for (uint32_t h = 0; h < height; h++)
            for (uint32_t w = 0; w < width; w++) {
                uint8_t r, g, b, a;
                stream >> b >> g >> r >> a;
                tmp.get(w, height - h - 1).m_color = PixmapColor(r, g, b, a);
            }

        bmp.m_pixmap = std::make_shared<Pixmap>(std::move(tmp));
        return fromPixmap(std::move(bmp));
    }
    if (width == 0 || height == 0) {
        throw std::runtime_error("Invalid PCX.");
    }

    if (width * height * 3 == size && fileSize == size + 12) { // RGB no pallete
        Pixmap tmp(width, height);
        for (uint32_t h = 0; h < height; h++)
            for (uint32_t w = 0; w < width; w++) {
                uint8_t r = 0, g = 0, b = 0;
                stream >> b >> g >> r;
                tmp.get(w, height - h - 1).m_color = PixmapColor(r, g, b, 255);
            }

        bmp.m_pixmap = std::make_shared<Pixmap>(std::move(tmp));
        return fromPixmap(std::move(bmp));
    }

    // PAL8 format. Pallete in the end of file.
    std::vector<std::vector<uint8_t>> pal8(height);

    Pixmap tmp(width, height);
    for (uint32_t h = 0; h < height; h++) {
        pal8[h].resize(width);
        for (uint32_t w = 0; w < width; w++) {
            stream >> pal8[h][w];
        }
    }
    std::vector<PixmapColor> palette(256);
    for (size_t i = 0; i < palette.size(); i++) {
        uint8_t r = 0, g = 0, b = 0;
        stream >> r >> g >> b;
        palette[i] = PixmapColor(r, g, b, 255);
    }
    for (uint32_t h = 0; h < height; h++) {
        for (uint32_t w = 0; w < width; w++)
            tmp.get(w, h).m_color = palette[pal8[h][w]];
    }
    bmp.m_pixmap = std::make_shared<Pixmap>(std::move(tmp));
    fromPixmap(std::move(bmp));
}

void SpriteFile::readBMP(const Mernel::std_path& bmpFilePath)
{
    BitmapFile bmp;
    bmp.m_pixmap = std::make_shared<Pixmap>();
    bmp.m_pixmap->loadBmp(bmpFilePath);
    fromPixmap(std::move(bmp));
}

void SpriteFile::readMSK(Mernel::ByteOrderDataStreamReader& stream)
{
    stream >> m_mask.m_width >> m_mask.m_height;
    m_mask.m_draw1.resize(48);
    m_mask.m_draw2.resize(48);
    stream.readBits(m_mask.m_draw1);
    stream.readBits(m_mask.m_draw2);
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

void SpriteFile::fromPixmap(BitmapFile data)
{
    m_embeddedBitmapData = false;
    m_bitmaps.resize(1);
    m_bitmaps[0] = std::move(data);
    const auto w = m_bitmaps[0].m_pixmap->m_size.m_width;
    const auto h = m_bitmaps[0].m_pixmap->m_size.m_height;

    m_bitmaps[0].m_width  = w;
    m_bitmaps[0].m_height = h;
    m_boundaryHeight      = h;
    m_boundaryWidth       = w;

    m_groups.resize(1);
    Group& group = m_groups[0];
    group.m_frames.resize(1);
    Frame& frame        = group.m_frames[0];
    frame.m_bitmapIndex = 0;
    frame.m_bitmapWidth = frame.m_boundaryWidth = w;
    frame.m_bitmapHeight = frame.m_boundaryHeight = h;
    frame.m_hasBitmap                             = true;
}

void SpriteFile::fromPixmapList(std::vector<BitmapFile> data, bool singleGroup)
{
    m_embeddedBitmapData = false;
    m_bitmaps            = std::move(data);
    const auto w         = m_bitmaps[0].m_pixmap->m_size.m_width;
    const auto h         = m_bitmaps[0].m_pixmap->m_size.m_height;

    m_bitmaps[0].m_width  = w;
    m_bitmaps[0].m_height = h;
    m_boundaryHeight      = h;
    m_boundaryWidth       = w;

    if (singleGroup) {
        m_groups.resize(1);
        Group& group    = m_groups[0];
        group.m_groupId = 0;
        group.m_frames.resize(m_bitmaps.size());

        for (int i = 0; Frame & frame : group.m_frames) {
            frame.m_bitmapIndex = size_t(i);
            frame.m_bitmapWidth = frame.m_boundaryWidth = w;
            frame.m_bitmapHeight = frame.m_boundaryHeight = h;
            frame.m_hasBitmap                             = true;
            i++;
        }
    } else {
        m_groups.resize(m_bitmaps.size());
        for (int i = 0; Group & group : m_groups) {
            group.m_groupId = i;
            group.m_frames.resize(1);
            Frame& frame        = group.m_frames[0];
            frame.m_bitmapIndex = size_t(i);
            frame.m_bitmapWidth = frame.m_boundaryWidth = w;
            frame.m_bitmapHeight = frame.m_boundaryHeight = h;
            frame.m_hasBitmap                             = true;
            i++;
        }
    }
}

void SpriteFile::saveBitmapsData(const Mernel::std_path& jsonFilePath) const
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

void SpriteFile::loadBitmapsData(const Mernel::std_path& jsonFilePath)
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

void SpriteFile::unpackPalette(bool extendPalette)
{
    if (m_format != BinaryFormat::DEF)
        return;
    BitmapFile::Palette paletteExtended;
    if (extendPalette) {
        g_extendedColors.extendPalette(m_palette, paletteExtended, m_defType);
    }

    for (auto& bitmap : m_bitmaps)
        bitmap.unpackPalette(m_palette, paletteExtended);
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
    g_extendedColors.fillReverseIndex(palette, m_defType);
    std::vector<std::pair<BitmapFile::Pixel, size_t>> counter;
    for (auto&& [pix, cnt] : palette.m_counter) {
        counter.push_back({ pix, cnt });
    }

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

void SpriteFile::setEmbeddedData(bool flag, bool extendPalette)
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
        unpackPalette(extendPalette);

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

    if (m_bitmaps.size() == 1)
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
        for (Frame& frame : group.m_frames) {
            if (!frame.m_hasBitmap)
                continue;
            g.m_maxHeight = std::max(g.m_maxHeight, frame.m_bitmapHeight);
            g.m_totalWidth += frame.m_bitmapWidth;
            frame.m_bitmapOffsetY = totalHeight;
        }
        totalHeight += g.m_maxHeight;
        maxWidth = std::max(maxWidth, g.m_totalWidth);
    }

    Pixmap out(maxWidth, totalHeight);
    out.fill({});

    for (Group& group : m_groups) {
        int x = 0;
        for (Frame& frame : group.m_frames) {
            if (!frame.m_hasBitmap)
                continue;
            Pixmap pix            = *m_bitmaps[frame.m_bitmapIndex].m_pixmap.get();
            frame.m_bitmapIndex   = 0;
            frame.m_bitmapOffsetX = x;
            x += frame.m_bitmapWidth;

            Painter painter(&out);
            painter.drawPixmap(PixmapPoint(frame.m_bitmapOffsetX, frame.m_bitmapOffsetY), pix);
        }
    }

    m_bitmaps.clear();
    m_bitmaps.resize(1);
    m_bitmaps[0].m_pixmap = std::make_shared<Pixmap>(std::move(out));
    m_bitmaps[0].m_width  = m_bitmaps[0].m_pixmap->m_size.m_width;
    m_bitmaps[0].m_height = m_bitmaps[0].m_pixmap->m_size.m_height;
}

void SpriteFile::saveGuiSprite(const Mernel::std_path& jsonFilePath, const Mernel::PropertyTree& handlers)
{
    const bool doAnimatePalette = handlers.contains("animatePalette");
    if (m_embeddedBitmapData && !doAnimatePalette)
        throw std::runtime_error("Only split bitmaps can be saved");

    if (doAnimatePalette) {
        uncompress();
        m_embeddedBitmapData = false;
    }

    if (handlers.contains("unpack")) {
        auto formatFileWithIndex = [](std::string tpl, int index) {
            const std::string from      = "%1";
            const std::string to        = std::to_string(index);
            size_t            start_pos = tpl.find(from);
            if (start_pos != std::string::npos)
                tpl.replace(start_pos, from.length(), to);
            return tpl;
        };

        const auto parentFolder = jsonFilePath.parent_path();

        for (const auto& task : handlers["unpack"].getList()) {
            const std::string tpl        = task["template"].getScalar().toString();
            const std::string subdir     = task["subdir"].getScalar().toString();
            const auto        taskFolder = parentFolder / subdir;

            const int idOffset   = task["idOffset"].getScalar().toInt();
            const int startFrame = task["startFrame"].getScalar().toInt();
            const int endFrame   = task["endFrame"].getScalar().toInt();

            const Group& group = m_groups[0];

            for (int f = startFrame, i = 0; f <= endFrame; ++f, ++i) {
                const int              id         = idOffset + i;
                const std::string      filename   = formatFileWithIndex(tpl, id);
                const std::string      filenameJs = filename + ".fhsprite.json";
                const Mernel::std_path path       = taskFolder / Mernel::string2path(filenameJs);
                const Frame&           frame      = group.m_frames[f];
                if (!frame.m_hasBitmap)
                    continue;

                SpriteFile splitted;
                splitted.fromPixmap(m_bitmaps[frame.m_bitmapIndex]);
                splitted.m_groups[0].m_frames[0].m_paddingLeft    = frame.m_paddingLeft;
                splitted.m_groups[0].m_frames[0].m_paddingTop     = frame.m_paddingTop;
                splitted.m_groups[0].m_frames[0].m_boundaryHeight = frame.m_boundaryHeight;
                splitted.m_groups[0].m_frames[0].m_boundaryWidth  = frame.m_boundaryWidth;
                splitted.m_boundaryHeight                         = frame.m_boundaryHeight;
                splitted.m_boundaryWidth                          = frame.m_boundaryWidth;
                splitted.saveGuiSprite(path, {});
            }
        }
    }
    if (handlers.contains("transpose")) {
        const Group            groupOrig = m_groups[0];
        AnimationPaletteConfig config; // "animatePalette": {"count": 9, "shifts": [{"from": 246, "len": 9}] } } ,
        if (doAnimatePalette) {
            const auto& dataPal    = handlers["animatePalette"];
            config.m_variantsCount = dataPal["count"].getScalar().toInt();
            for (const auto& s : dataPal["shifts"].getList()) {
                config.m_shifts.push_back(AnimationPaletteShift{ (int) s["from"].getScalar().toInt(), (int) s["len"].getScalar().toInt() });
            }
        }
        BitmapFile::Palette paletteExtended;
        if (doAnimatePalette) {
            g_extendedColors.extendPalette(m_palette, paletteExtended, m_defType);
        }

        m_groups.clear();
        for (int groupId = 0; const auto& frame : groupOrig.m_frames) {
            Group groupNew;
            groupNew.m_groupId = groupId;
            if (doAnimatePalette) {
                auto palette = m_palette;

                for (int i = 0; i < config.m_variantsCount; ++i) {
                    Frame      frameCopy = frame;
                    BitmapFile bmp       = m_bitmaps[frame.m_bitmapIndex];
                    bmp.unpackPalette(palette, paletteExtended);
                    bmp.toPixmapQt();
                    frameCopy.m_bitmapIndex = m_bitmaps.size();
                    m_bitmaps.push_back(std::move(bmp));
                    groupNew.m_frames.push_back(frameCopy);
                    config.shiftPalette(palette);
                }
            } else {
                groupNew.m_frames.push_back(frame);
            }

            m_groups.push_back(std::move(groupNew));
            groupId++;
        }
    }

    mergeBitmaps();

    Mernel::std_fs::create_directories(jsonFilePath.parent_path());

    assert(m_bitmaps.size() == 1);

    int boundaryPadWidth  = 0;
    int boundaryPadHeight = 0;
    if (handlers.contains("pad")) {
        const auto& pad = handlers["pad"];
        if (pad.contains("right"))
            boundaryPadWidth = pad["right"].getScalar().toInt();
        if (pad.contains("bottom"))
            boundaryPadHeight = pad["bottom"].getScalar().toInt();
    }

    Gui::Sprite uiSprite;
    uiSprite.m_bitmap       = *m_bitmaps[0].m_pixmap.get();
    uiSprite.m_boundarySize = { m_boundaryWidth + boundaryPadWidth, m_boundaryHeight + boundaryPadHeight };
    std::vector<const Frame*> headerOrderFrames;
    for (const Group& group : m_groups) {
        Gui::Sprite::Group uiGroup;
        uiGroup.m_groupId = group.m_groupId;

        for (const Frame& frame : group.m_frames) {
            headerOrderFrames.push_back(&frame);
            Gui::Sprite::FrameImpl uiFrame;
            const Frame*           srcframe = &frame;
            if (srcframe->m_isDuplicate) {
                srcframe = headerOrderFrames[srcframe->m_dupHeaderIndex];
            }
            uiFrame.m_bitmapOffset = { srcframe->m_bitmapOffsetX, srcframe->m_bitmapOffsetY };
            uiFrame.m_hasBitmap    = srcframe->m_hasBitmap;
            uiFrame.m_bitmapSize   = { srcframe->m_bitmapWidth, srcframe->m_bitmapHeight };
            uiFrame.m_boundarySize = { srcframe->m_boundaryWidth + boundaryPadWidth, srcframe->m_boundaryHeight + boundaryPadHeight };
            uiFrame.m_padding      = { srcframe->m_paddingLeft, srcframe->m_paddingTop };

            uiGroup.m_frames.push_back(std::move(uiFrame));
        }

        if (handlers.isMap()) {
            for (const auto& [key, routineParam] : handlers.getMap()) {
                if (key == "battle_unit") {
                    postProcessSpriteGroupBattler(group.m_groupId, uiGroup, routineParam);
                }
            }
        }

        uiSprite.m_groups[group.m_groupId] = std::move(uiGroup);
    }
    if (handlers.isMap()) {
        for (const auto& [key, routineParam] : handlers.getMap()) {
            if (key == "flip_vertical") {
                uiSprite.m_bitmap.flipVertical();
            } else if (key == "fix_colors") {
                const std::map<std::string, PixmapColor> names{
                    { "tr", PixmapColor(0, 0, 0, 0) },
                    { "key", PixmapColor(0, 0, 0, 1) },
                    { "shadow", PixmapColor(0, 0, 0, 0x80) },
                    { "shadowborder", PixmapColor(0, 0, 0, 0x40) },
                };
                for (auto& [name, destColor] : names) {
                    if (!routineParam.contains(name))
                        continue;
                    PixmapColor src(routineParam[name].getScalar().toString());

                    replaceColors(uiSprite.m_bitmap, src, destColor);
                }
                *m_bitmaps[0].m_pixmap.get() = uiSprite.m_bitmap;
            }
        }
    }
    uiSprite.m_mask = Gui::ISprite::SpriteSequenceMask{
        .m_width  = m_mask.m_width,
        .m_height = m_mask.m_height,
        .m_draw1  = m_mask.m_draw1,
        .m_draw2  = m_mask.m_draw2,
    };

    uiSprite.save(jsonFilePath);
}

}
