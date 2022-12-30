/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "DefFile.hpp"

#include <QVector>
#include <QMap>
#include <QColor>
#include <QtEndian>
#include <QPainter>
#include <QDebug>

#include <set>

#include <cstdint>

namespace FreeHeroes::Conversion {

namespace {

enum class DefType : uint32_t
{
    SPELL        = 0x40,
    SPRITE       = 0x41,
    CREATURE     = 0x42,
    MAP          = 0x43,
    MAP_HERO     = 0x44,
    TERRAIN      = 0x45,
    CURSOR       = 0x46,
    INTERFACE    = 0x47,
    SPRITE_FRAME = 0x48,
    BATTLE_HERO  = 0x49
};

const QMap<DefType, QString> typeStr{
    { DefType::SPELL, "BattleSpells" },
    { DefType::SPRITE, "Sprite" },
    { DefType::CREATURE, "BattleSprite" },
    { DefType::MAP, "Adventure" },
    { DefType::MAP_HERO, "AdventureHero" },
    { DefType::TERRAIN, "AdventureTerrain" },
    { DefType::CURSOR, "Cursor" },
    { DefType::INTERFACE, "Interface" },
    { DefType::SPRITE_FRAME, "SpriteFrame" },
    { DefType::BATTLE_HERO, "BattleHero" },
};

}

struct Palette {
    QVector<QRgb> colors = QVector<QRgb>(256);

    // palette shift used for animanted terrain like water.
    void makeShift(const AnimationPaletteConfig& animConfig)
    {
        QVector<QRgb> palCopy = colors;
        for (const AnimationPaletteShift& shift : animConfig.shifts) {
            for (int i = 0; i < shift.count; ++i)
                palCopy[shift.from + (i + 1) % shift.count] = colors[shift.from + i];
        }
        colors = palCopy;
    }

    void init(uint32_t type)
    {
        //First 8 colors in def palette used for transparency
        // clang-format off
        static const QRgb H3Palette[8] =
        {
            //qRgba(   rand()%128 + 128,   rand()%128+ 128,  rand()%128+ 128,   128),// debug sprites
            qRgba(   0,   0,   0,  0),// 100% - transparency
            qRgba(   0,   0,   0,  32),//  75% - shadow border,
            qRgba(   0,   0,   0,  64),// TODO: find exact value
            qRgba(   0,   0,   0,   0),// TODO: for transparency
            qRgba(   0,   0,   0, 128),//  50% - shadow body
            qRgba(   0,   0,   0,   0),// 100% - selection highlight
            qRgba(   0,   0,   0, 128),//  50% - shadow body   below selection
            qRgba(   0,   0,   0,  64),//  75% - shadow border below selection
        };
        // clang-format on

        switch (static_cast<DefType>(type)) {
            case DefType::SPELL:
                colors[0] = H3Palette[0];
                break;
            case DefType::SPRITE:
            case DefType::SPRITE_FRAME:
                for (uint32_t i = 0; i < 8; i++)
                    colors[i] = H3Palette[i];
                break;
            case DefType::CREATURE:
                colors[0] = H3Palette[0];
                colors[1] = H3Palette[1];
                colors[4] = H3Palette[4];
                colors[5] = H3Palette[5];
                colors[6] = H3Palette[6];
                colors[7] = H3Palette[7];
                break;
            case DefType::MAP:
            case DefType::MAP_HERO:
                colors[0] = H3Palette[0];
                colors[1] = H3Palette[1];
                colors[2] = H3Palette[2];
                colors[3] = H3Palette[3];
                colors[4] = H3Palette[4];

                colors[5] = qRgba(0, 0, 0, 1); // owner;
                break;
            case DefType::TERRAIN:
                colors[0] = H3Palette[0];
                colors[1] = H3Palette[1];
                colors[2] = H3Palette[2];
                colors[3] = H3Palette[3];
                colors[4] = H3Palette[4];
                break;
            case DefType::CURSOR:
                colors[0] = H3Palette[0];
                break;
            case DefType::INTERFACE:
                colors[0] = H3Palette[0];
                colors[1] = H3Palette[1];
                colors[4] = H3Palette[4];
                //player colors handled separately
                //TODO: disallow colorizing other def types
                break;
            case DefType::BATTLE_HERO:
                colors[0] = H3Palette[0];
                colors[1] = H3Palette[1];
                colors[4] = H3Palette[4];
                break;
            default:
                // @todo: error reporter interface here!
                //logAnim->error("Unknown def type %d", type);
                break;
        }

        if (0) {
            colors[0] = qRgba(255, 0, 0, 128);
            colors[1] = qRgba(230, 0, 0, 128);
            colors[2] = qRgba(210, 0, 0, 128);
            colors[3] = qRgba(190, 0, 0, 128);
            colors[4] = qRgba(170, 0, 0, 128);
            colors[5] = qRgba(150, 0, 0, 128);
            colors[6] = qRgba(130, 0, 0, 128);
            colors[7] = qRgba(120, 0, 0, 128);
        }
    }
};

namespace {

class PixmapDataParser {
    int          m_y = 0;
    int          m_x = 0;
    QSize        m_spriteSize;
    QImage       m_tmpImage;
    QDataStream& m_ds;

public:
    PixmapDataParser(QDataStream&   ds,
                     QSize          spriteSize,
                     const Palette& pal)
        : m_ds(ds)
    {
        m_tmpImage = QImage(spriteSize, QImage::Format_Indexed8);
        m_tmpImage.setColorTable(pal.colors);
        m_spriteSize = spriteSize;
    }

    //load size raw pixels from data
    inline void Load(size_t size)
    {
        for (size_t i = 0; i < size; ++i) {
            uint8_t byte;
            m_ds >> byte;
            LoadPixel(byte);
        }
    }

    inline void Load(size_t size, uint8_t color)
    {
        for (size_t i = 0; i < size; ++i)
            LoadPixel(color);
    }

    inline void LoadPixel(uint8_t color)
    {
        assert(m_x < m_tmpImage.width());
        m_tmpImage.setPixel(m_x, m_y, color);
        m_x++;
        //  if (color == 5)
        //       m_flagColorUsed = true;
    }
    inline void EndLine()
    {
        m_y++;
        m_x = 0;
    }

    void LoadLine(bool compressedLength, const int baseOffset = 0)
    {
        int TotalRowLength = 0;

        while (TotalRowLength < m_spriteSize.width()) {
            uint8_t  segmentType;
            uint8_t  code;
            uint32_t length;
            bool     isRaw;
            if (compressedLength) {
                m_ds >> segmentType;
                code   = segmentType / 32;
                length = (segmentType & 31) + 1;
                isRaw  = code == 7;
            } else {
                uint8_t lengthTmp;
                m_ds >> segmentType;
                m_ds >> lengthTmp;
                code   = segmentType;
                length = lengthTmp + 1;
                isRaw  = segmentType == 0xFF;
            }

            if (isRaw)
                this->Load(length);
            else
                this->Load(length, code);

            TotalRowLength += length;
        }
    }

    QPixmap LoadAll(int format)
    {
        const auto baseOffset = m_ds.device()->pos();
        switch (format) {
            case 0:
            {
                //pixel data is not compressed, copy data to surface
                for (int i = 0; i < m_spriteSize.height(); i++) {
                    this->Load(m_spriteSize.width());
                    this->EndLine();
                }
                break;
            }
            case 1:
            {
                //for each line we have offset of pixel data
                QVector<uint32_t> offsets(m_spriteSize.height());
                for (uint32_t& offset : offsets)
                    m_ds >> offset;

                for (int i = 0; i < m_spriteSize.height(); i++) {
                    //get position of the line
                    auto currentOffset = baseOffset + offsets[i];
                    m_ds.device()->seek(currentOffset);

                    this->LoadLine(false);
                    this->EndLine();
                }
                break;
            }
            case 2:
            {
                uint16_t offset;
                m_ds >> offset;
                m_ds.device()->seek(baseOffset + offset);

                for (int i = 0; i < m_spriteSize.height(); i++) {
                    this->LoadLine(true);
                    this->EndLine();
                }
                break;
            }
            case 3:
            {
                for (int i = 0; i < m_spriteSize.height(); i++) {
                    auto offset1 = i * 2 * (m_spriteSize.width() / 32);
                    m_ds.device()->seek(baseOffset + offset1);
                    uint16_t offset2;
                    m_ds >> offset2;
                    m_ds.device()->seek(baseOffset + offset2);

                    this->LoadLine(true, baseOffset);
                    this->EndLine();
                }
                break;
            }
            default:
                qWarning() << "unknown def pix format=" << format;
                break;
        }
        QPixmap result = QPixmap::fromImage(m_tmpImage.convertToFormat(QImage::Format_RGBA8888));
        return result;
    }
};

struct FrameTask {
    uint32_t offset;
    int      pallete;
};
struct GroupOffsetList {
    std::vector<FrameTask> tasks;
    int                    groupId;
};

} // namespace

DefParser::DefParser(const char* dataPtr, int size, const AnimationPaletteConfig& animConfig, const std::string& resourceName)
    : m_data(QByteArray::fromRawData(dataPtr, size))
    , m_resourceName(QString::fromStdString(resourceName))
    , m_animConfig(animConfig)
{
    m_dataBuffer.setData(m_data);
    m_dataBuffer.open(QIODevice::ReadOnly);

    QDataStream ds(&m_dataBuffer);
    ds.setByteOrder(QDataStream::LittleEndian);

    Palette m_palette;

    uint32_t type;
    ds >> type;
    if (type == 0x46323344U) { // 'D32F'
        loadHotA();
        //return;
    } else {
        loadSoD(type);
    }
}

DefParser::ParsedFrame DefParser::loadFrame(uint32_t offset, const Palette& palette)
{
    m_dataBuffer.seek(offset);
    QDataStream ds(&m_dataBuffer);
    ds.setByteOrder(QDataStream::LittleEndian);

    struct SpriteDef {
        uint32_t size;      // ignored! do not remove
        uint32_t format;    /// format in which pixel data is stored
        uint32_t fullWidth; /// full width and height of frame, including borders
        uint32_t fullHeight;
        uint32_t width; /// width and height of pixel data, borders excluded
        uint32_t height;
        int32_t  leftMargin;
        int32_t  topMargin;
    };

    SpriteDef sprite;
    ds
        >> sprite.size
        >> sprite.format
        >> sprite.fullWidth
        >> sprite.fullHeight
        >> sprite.width
        >> sprite.height
        >> sprite.leftMargin
        >> sprite.topMargin;
    uint32_t currentOffset = sizeof(SpriteDef);

    //special case for some "old" format defs (SGTWMTA.DEF and SGTWMTB.DEF)
    if (sprite.format == 1 && sprite.width > sprite.fullWidth && sprite.height > sprite.fullHeight) {
        sprite.leftMargin = 0;
        sprite.topMargin  = 0;
        sprite.width      = sprite.fullWidth;
        sprite.height     = sprite.fullHeight;

        currentOffset -= 16;
        m_dataBuffer.seek(m_dataBuffer.pos() - 16);
    }

    PixmapDataParser loader(ds, QSize(sprite.width, sprite.height), palette);

    ParsedFrame result;
    result.data           = loader.LoadAll(sprite.format);
    result.boundarySize   = QSize(sprite.fullWidth, sprite.fullHeight);
    result.paddingLeftTop = QPoint(sprite.leftMargin, sprite.topMargin);
    return result;
}

void DefParser::loadSoD(uint32_t type)
{
    m_type = typeStr.value(static_cast<DefType>(type), "OTHER");

    QDataStream ds(&m_dataBuffer);
    ds.setByteOrder(QDataStream::LittleEndian);

    Palette pal;

    ds.skipRawData(8); // int width, height;
    uint32_t totalBlocks;
    ds >> totalBlocks;

    for (uint32_t i = 0; i < 256; ++i) {
        uint8_t r, g, b;
        ds >> r >> g >> b;
        pal.colors[i] = qRgba(r, g, b, 255);
    }
    pal.init(type);

    std::set<uint32_t> allOffsets;

    std::vector<GroupOffsetList> groupsOffsetLists;
    const bool                   needPaletteShift = (m_animConfig.variantsCount >= 2);

    std::vector<int> groupIds(totalBlocks);

    for (uint32_t i = 0; i < totalBlocks; i++) {
        int32_t blockID;
        ds >> blockID;
        int32_t totalEntries;
        ds >> totalEntries;
        ds.skipRawData(8); //8 unknown bytes - skipping

        //13 bytes for name of every frame in this block - not used, skipping
        ds.skipRawData(13 * totalEntries);

        GroupOffsetList offsetList;
        offsetList.groupId = blockID;

        for (int frameIndex = 0; frameIndex < totalEntries; ++frameIndex) {
            uint32_t currOffset;
            ds >> currOffset;
            allOffsets.insert(currOffset);
            offsetList.tasks.push_back(FrameTask{ currOffset, 0 });
        }
        groupsOffsetLists.push_back(offsetList);
    }
    if (needPaletteShift) {
        assert(totalBlocks == 1);
        GroupOffsetList first = groupsOffsetLists[0];
        groupsOffsetLists.clear();
        int groupId = 0;
        for (FrameTask task : first.tasks) {
            GroupOffsetList tmp;
            for (int palleteIndex = 0; palleteIndex < m_animConfig.variantsCount; ++palleteIndex) {
                task.pallete = palleteIndex;
                tmp.tasks.push_back(task);
            }
            tmp.groupId = groupId++;
            groupsOffsetLists.push_back(tmp);
        }
    }
    using OffsetToFrameId = std::map<uint32_t, int>;
    std::map<int, OffsetToFrameId> palleteAndOffsetToFrameId;
    std::map<int, QSize>           frameIdToBoundaries;

    int frameId = 0;

    for (int palleteIndex = 0; palleteIndex < m_animConfig.variantsCount; ++palleteIndex) {
        for (uint32_t offset : allOffsets) {
            ParsedFrame info                                = loadFrame(offset, pal);
            info.frameId                                    = frameId++;
            frameIdToBoundaries[info.frameId]               = info.boundarySize;
            palleteAndOffsetToFrameId[palleteIndex][offset] = info.frameId;
            m_parsedFrames << info;
        }
        pal.makeShift(m_animConfig);
    }

    for (const GroupOffsetList& offsetList : groupsOffsetLists) {
        QList<int> ids;
        QSize      boundarySize;
        for (const FrameTask& task : offsetList.tasks) {
            const int id = palleteAndOffsetToFrameId.at(task.pallete).at(task.offset);
            ids << id;
            auto frameBoundary = frameIdToBoundaries[id];
            assert(boundarySize.isEmpty() || frameBoundary == boundarySize);
            boundarySize = frameBoundary;
        }
        m_parsedGroups << ParsedGroup{ offsetList.groupId, ids, boundarySize };
    }
    m_status = true;
}

void DefParser::loadHotA()
{
    m_type = "HotaSprite";

    QDataStream ds(&m_dataBuffer);
    ds.setByteOrder(QDataStream::LittleEndian);
    uint32_t version, count1, width, height;
    ds >> version >> count1 >> width >> height;
    assert(24 == count1);

    uint32_t u1, u2, u3;
    ds >> u1 >> u2 >> u3;
    assert(u1 == 1);
    assert(u2 == 8);
    assert(u3 == 1);

    uint32_t headerTotal, u5, frameCount, u7;
    ds >> headerTotal >> u5 >> frameCount >> u7;
    assert(u5 == 0);
    assert(headerTotal == 17 * frameCount + 16);
    assert(u7 == 4);

    ds.skipRawData(13 * frameCount); // image string names, like "frame001\0"

    ds.skipRawData(4 * frameCount); // offsets in file (not exact, like + 6 bytes - weird)

    struct SpriteDef {
        uint32_t format;
        int32_t  size;
        int32_t  fullWidth;
        int32_t  fullHeight;
        int32_t  width;
        int32_t  height;
        int32_t  leftMargin; // not sure
        int32_t  topMargin;  // not sure
        uint32_t u0;
        uint32_t u1;
    };

    ParsedGroup group;

    for (uint32_t i = 0; i < frameCount; ++i) {
        SpriteDef def;
        ds
            >> def.format
            >> def.size
            >> def.fullWidth
            >> def.fullHeight
            >> def.width
            >> def.height
            >> def.leftMargin
            >> def.topMargin
            >> def.u0
            >> def.u1;
        bool isEmpty = def.fullWidth == 0 || def.fullHeight == 0;
        if (isEmpty)
            assert(def.u0 == 0);
        else
            assert(def.u0 == 8);
        assert(def.u1 == 0);
        assert(def.size == def.width * def.height * 4);

        if (!isEmpty)
            assert(def.format == 32);

        QImage tmp(def.width, def.height, QImage::Format_RGBA8888);
        for (int32_t h = 0; h < def.height; h++)
            for (int32_t w = 0; w < def.width; w++) {
                uint8_t r, g, b, a;
                ds >> b >> g >> r >> a;
                tmp.setPixelColor(w, def.height - h - 1, QColor(r, g, b, a));
            }

        ParsedFrame info;
        info.frameId        = i;
        info.boundarySize   = QSize{ def.fullWidth, def.fullHeight };
        info.paddingLeftTop = QPoint{ def.leftMargin, def.topMargin };
        info.data           = QPixmap::fromImage(tmp);
        group.boundarySize  = info.boundarySize;
        group.frameIds << i;
        m_parsedFrames << info;
    }
    group.boundarySize = QSize(width, height);

    m_parsedGroups << group;

    m_status = true;
}

}
