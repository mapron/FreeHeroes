/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BitmapFile.hpp"

#include "MernelPlatform/ByteOrderStream.hpp"
#include "MernelPlatform/FsUtils.hpp"

namespace Mernel {
class PropertyTree;
}

namespace FreeHeroes {
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
        int    m_bitmapOffsetY = 0;

        std::string          m_bitmapFilename;
        std::vector<uint8_t> m_bitmapFilenamePad;

        int m_boundaryWidth  = 0;
        int m_boundaryHeight = 0;

        size_t m_binaryOrder    = 0;
        bool   m_isDuplicate    = false;
        size_t m_dupHeaderIndex = 0;
        size_t m_headerIndex    = 0;

        bool m_shortHeaderFormat = false;

        // VVV non serializable VVV
        uint32_t m_originalOffset = 0;
    };
    struct Group {
        int                m_groupId = 0;
        int                m_unk1    = 0;
        int                m_unk2    = 0;
        std::vector<Frame> m_frames;
    };

    struct Mask {
        uint8_t              m_width  = 0;
        uint8_t              m_height = 0;
        std::vector<uint8_t> m_draw1;
        std::vector<uint8_t> m_draw2;
    };

    BinaryFormat m_format         = BinaryFormat::Invalid;
    DefType      m_defType        = DefType::Invalid;
    int          m_boundaryWidth  = 0;
    int          m_boundaryHeight = 0;

    std::vector<BitmapFile> m_bitmaps;

    std::vector<Group> m_groups;

    BitmapFile::Palette m_palette;

    std::vector<std::string> m_tralilingData;

    Mask m_mask;

    bool m_embeddedBitmapData = true;

    void detectFormat(const Mernel::std_path& path, Mernel::ByteOrderDataStreamReader& stream);

    void readBinary(Mernel::ByteOrderDataStreamReader& stream);
    void writeBinary(Mernel::ByteOrderDataStreamWriter& stream) const;

    void readBinaryPCX(Mernel::ByteOrderDataStreamReader& stream);
    void readBMP(const Mernel::std_path& bmpFilePath);

    void readMSK(Mernel::ByteOrderDataStreamReader& stream);

    void toJson(Mernel::PropertyTree& data) const;
    void fromJson(const Mernel::PropertyTree& data);

    void fromPixmap(BitmapFile data);
    void fromPixmapList(std::vector<BitmapFile> data);

    void saveBitmapsData(const Mernel::std_path& jsonFilePath) const;
    void loadBitmapsData(const Mernel::std_path& jsonFilePath);

    void uncompress();
    void compressToOriginal();

    void unpackPalette(bool extendPalette);
    void makePalette();

    void setEmbeddedData(bool flag, bool extendPalette);

    void mergeBitmaps();

    void saveGuiSprite(const Mernel::std_path& jsonFilePath, const Mernel::PropertyTree& handlers);
};

}
