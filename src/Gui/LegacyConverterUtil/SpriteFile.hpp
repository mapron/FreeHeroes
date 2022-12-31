/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BitmapFile.hpp"

#include "ByteOrderStream.hpp"
#include "FsUtils.hpp"

namespace FreeHeroes {
class PropertyTree;
class SpriteFile {
public:
    enum class BinaryFormat
    {
        Invalid,
        DEF,
        DEF32,
        BMP,
        PCX,
    };
    enum class DefType
    {
        Invalid          = 0,
        BattleSpells     = 0x40,
        Sprite           = 0x41,
        BattleSprite     = 0x42,
        AdventureItem    = 0x43,
        AdventureHero    = 0x44,
        AdventureTerrain = 0x45,
        Cursor           = 0x46,
        Interface        = 0x47,
        SpriteFrame      = 0x48,
        BattleHero       = 0x49,
    };

    struct Frame {
        int m_paddingLeft = 0;
        int m_paddingTop  = 0;

        bool   m_hasBitmap     = false;
        size_t m_bitmapIndex   = 0;
        int    m_bitmapWidth   = 0;
        int    m_bitmapHeight  = 0;
        int    m_bitmapOffsetX = 0;

        std::string          m_bitmapFilename;
        std::vector<uint8_t> m_bitmapFilenamePad;

        int m_boundaryWidth  = 0;
        int m_boundaryHeight = 0;

        int m_group = 0;

        size_t m_binaryOrder    = 0;
        bool   m_isDuplicate    = false;
        size_t m_dupHeaderIndex = 0;
        size_t m_headerIndex    = 0;

        bool m_shortHeaderFormat = false;

        // VVV non serializable VVV
        uint32_t m_originalOffset = 0;
    };
    struct Group {
        int                m_bitmapOffsetY = 0;
        int                m_groupId       = 0;
        int                m_unk1          = 0;
        int                m_unk2          = 0;
        std::vector<Frame> m_frames;
    };

    BinaryFormat m_format         = BinaryFormat::Invalid;
    DefType      m_defType        = DefType::Invalid;
    int          m_boundaryWidth  = 0;
    int          m_boundaryHeight = 0;

    std::vector<BitmapFile> m_bitmaps;

    std::vector<Group> m_groups;

    BitmapFile::Palette m_palette;

    std::vector<std::string> m_tralilingData;

    bool m_embeddedBitmapData = true;

    void detectFormat(const Core::std_path& path, ByteOrderDataStreamReader& stream);

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;

    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);

    void saveBitmapsData(const Core::std_path& jsonFilePath) const;
    void loadBitmapsData(const Core::std_path& jsonFilePath);

    void uncompress();
    void compressToOriginal();

    void unpackPalette();
    void makePalette();

    void setEmbeddedData(bool flag);

    void mergeBitmaps();
};

}
