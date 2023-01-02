/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "SpriteSerialization.hpp"

#include "MernelPlatform/ByteOrderStream.hpp"

#include <QVector>
#include <QBuffer>
#include <QSet>

namespace FreeHeroes::Conversion {

struct Palette;

struct AnimationPaletteShift {
    int from;
    int count;
};
struct AnimationPaletteConfig {
    QVector<AnimationPaletteShift> shifts;
    int                            variantsCount = 1;
};

class DefParser {
public:
    DefParser(const char* dataPtr, int size, const AnimationPaletteConfig& animConfig, const std::string& resourceName);
    ~DefParser() = default;

    struct ParsedFrame {
        QPoint  paddingLeftTop;
        QPixmap data;
        QSize   boundarySize;
        int     frameId = 0;
    };

    struct ParsedGroup {
        int        groupId = 0;
        QList<int> frameIds;
        QSize      boundarySize;
    };

    const QVector<ParsedFrame>& getFrames() const { return m_parsedFrames; }

    const QVector<ParsedGroup>& getGroups() const { return m_parsedGroups; }

    bool status() const { return m_status; }

    QString type() const { return m_type; }

private:
    QByteArray                   m_data;
    QString                      m_resourceName;
    QBuffer                      m_dataBuffer;
    const AnimationPaletteConfig m_animConfig;

    QVector<ParsedFrame> m_parsedFrames;
    QVector<ParsedGroup> m_parsedGroups;

    bool           m_status = false;
    QString        m_type;
    QSet<uint32_t> m_usedFormats;

    ParsedFrame loadFrame(uint32_t offset, const Palette& palette);

    void loadSoD(uint32_t type);
    void loadHotA();
};

}
