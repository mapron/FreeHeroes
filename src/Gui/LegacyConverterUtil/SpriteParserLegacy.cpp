/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpriteParserLegacy.hpp"
#include "DefFile.hpp"

#include "MernelPlatform/FileIOUtils.hpp"

#include "IResourceLibrary.hpp"

#include <QFile>
#include <QDebug>

#include <array>
#include <set>

inline Q_DECL_PURE_FUNCTION uint qHash(const QPoint& key, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(QPair{ key.x(), key.y() }, seed);
}

namespace FreeHeroes::Conversion {
using namespace Core;
using namespace Gui;
using namespace Mernel;

struct color_compare {
    bool operator()(QColor const& a, QColor const& b) const noexcept { return a.rgba() < b.rgba(); }
};
using QColorSet = std::set<QColor, color_compare>;

namespace {

struct UnpackTask {
    std::string tpl;
    std::string subdir;
    int         idOffset   = 0;
    int         startFrame = 0;
    int         endFrame   = 0;
};
using UnpackTaskList = std::vector<UnpackTask>;

AnimationPaletteConfig getConfigForResource(const std_path& resource)
{
    AnimationPaletteConfig cfg;
    if (resource == "lavatl")
        cfg = AnimationPaletteConfig{ { { 246, 9 } }, 9 };
    else if (resource == "watrtl")
        cfg = AnimationPaletteConfig{ { { 229, 12 }, { 242, 14 } }, 84 };
    else if (resource == "clrrvr")
        cfg = AnimationPaletteConfig{ { { 183, 12 }, { 195, 6 } }, 12 };
    else if (resource == "mudrvr")
        cfg = AnimationPaletteConfig{ { { 228, 12 }, { 183, 6 }, { 240, 6 } }, 12 };
    else if (resource == "lavrvr")
        cfg = AnimationPaletteConfig{ { { 240, 9 } }, 9 };
    return cfg;
}

void floodFillImageFromEdge(QImage& img, const QColorSet& keyColors, const QColor& destColor)
{
    const int maxW        = img.width();
    const int maxH        = img.height();
    auto      isMaskColor = [&img, &keyColors](const QPoint& p) {
        return keyColors.contains(img.pixelColor(p));
    };

    auto getAdjacentSet = [maxW, maxH](const QPoint& start) {
        QSet<QPoint> res;
        const int    x  = start.x();
        const int    y  = start.y();
        const int    w1 = x > 0 ? -1 : 0;
        const int    w2 = x < maxW - 1 ? +1 : 0;
        const int    h1 = y > 0 ? -1 : 0;
        const int    h2 = y < maxH - 1 ? +1 : 0;
        for (int w = w1; w <= w2; ++w) {
            for (int h = h1; h <= h2; ++h)
                res << QPoint{ x + w, y + h };
        }
        return res;
    };

    auto floodFill = [&img, &isMaskColor, &getAdjacentSet, &destColor](const QSet<QPoint>& start) {
        QSet<QPoint> edge = start;
        while (!edge.empty()) {
            QSet<QPoint> nextEdge;
            for (const auto edgePos : edge) {
                if (!isMaskColor(edgePos))
                    continue;

                img.setPixelColor(edgePos, destColor);
                QSet<QPoint> nextEdgeTmp = getAdjacentSet(edgePos);
                for (const auto edgePosNext : nextEdgeTmp) {
                    if (isMaskColor(edgePosNext))
                        nextEdge << edgePosNext;
                }
            }
            edge = nextEdge;
        }
    };
    QSet<QPoint> floodStart;
    for (int h = 0; h < img.height(); ++h) {
        if (isMaskColor({ 0, h }))
            floodStart << QPoint{ 0, h };
        if (isMaskColor({ img.width() - 1, h }))
            floodStart << QPoint{ img.width() - 1, h };
    }
    for (int w = 0; w < img.width(); ++w) {
        if (isMaskColor({ w, 0 }))
            floodStart << QPoint{ w, 0 };
        if (isMaskColor({ w, img.height() - 1 }))
            floodStart << QPoint{ w, img.height() - 1 };
    }
    floodFill(floodStart);
}

void replaceImageColors(QImage& img, const QColorSet& keyColors, const QColor& destColor)
{
    for (int h = 0; h < img.height(); ++h) {
        for (int w = 0; w < img.width(); ++w) {
            const QColor c = img.pixelColor(w, h);
            if (keyColors.contains(c))
                img.setPixelColor(w, h, destColor);
        }
    }
}

void postProcessSpriteMakeTransparent(SpriteSequence& seq, const PropertyTree& params)
{
    std::set<int> floodIndexes, trWhiteIndexes;
    if (params.contains("flood")) {
        for (const auto& val : params.getMap().at("flood").getList())
            floodIndexes.insert(val.getScalar().toInt());
    }
    if (params.contains("tr_to_white")) {
        for (const auto& val : params.getMap().at("tr_to_white").getList())
            trWhiteIndexes.insert(val.getScalar().toInt());
    }
    int       frameIndex = 0;
    QColorSet keyColors{ QColor(255, 255, 255), QColor(250, 250, 250), QColor(245, 245, 245) };

    for (auto& frame : seq.frames) {
        QPixmap& p = frame.frame;
        if (p.isNull())
            continue;
        QImage imgOldPixFmt = p.toImage();
        QImage img(imgOldPixFmt.width(), imgOldPixFmt.height(), QImage::Format_RGBA8888);
        for (int h = 0; h < img.height(); ++h) {
            for (int w = 0; w < img.width(); ++w) {
                img.setPixelColor(w, h, imgOldPixFmt.pixelColor(w, h));
            }
        }
        auto keyColorsFrame = keyColors;

        std::map<QColor, int, color_compare> cornerColors;
        cornerColors[img.pixelColor(0, 0)]++;
        cornerColors[img.pixelColor(0, img.height() - 1)]++;
        cornerColors[img.pixelColor(img.width() - 1, 0)]++;
        cornerColors[img.pixelColor(img.width() - 1, img.height() - 1)]++;
        if (cornerColors.size() == 1) {
            keyColorsFrame.insert(cornerColors.begin()->first);
        }
        if (cornerColors.size() == 2) {
            auto aClr = cornerColors.begin()->first;
            auto aCnt = cornerColors.begin()->second;
            auto bClr = cornerColors.rbegin()->first;
            auto bCnt = cornerColors.rbegin()->second;
            if (aCnt != bCnt) {
                keyColorsFrame.insert(aCnt > bCnt ? aClr : bClr);
            }
        }
        if (cornerColors.size() == 3) {
            for (const auto& [clr, cnt] : cornerColors) {
                if (cnt == 2)
                    keyColorsFrame.insert(clr);
            }
        }
        if (trWhiteIndexes.contains(frameIndex)) {
            replaceImageColors(img, { Qt::transparent }, Qt::white);
        }
        if (floodIndexes.contains(frameIndex))
            floodFillImageFromEdge(img, keyColorsFrame, Qt::transparent);
        else
            replaceImageColors(img, keyColorsFrame, Qt::transparent);

        p = QPixmap::fromImage(img);
        frameIndex++;
    }
}

void postProcessSpriteFlipVertical(SpriteSequence& seq)
{
    for (auto& frame : seq.frames) {
        QPixmap& p = frame.frame;
        if (p.isNull())
            continue;
        p = p.transformed(QTransform().scale(1, -1));
    }
}

void postProcessSpriteGroupBattler(int groupIndex, SpriteSequence& seq, const PropertyTree& params)
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

    seq.params.animationCycleDuration = getAnimDuration(seq.frames.size());
    if (!actionPoints.empty()) {
        if (type == BattleAnimation::RangedUp)
            seq.params.actionPoint = { actionPoints[0], actionPoints[1] };
        else if (type == BattleAnimation::RangedCenter)
            seq.params.actionPoint = { actionPoints[2], actionPoints[3] };
        else if (type == BattleAnimation::RangedDown)
            seq.params.actionPoint = { actionPoints[4], actionPoints[5] };

        if (!seq.params.actionPoint.isNull())
            seq.params.specialFrameIndex = special;
    }
    for (auto& frame : seq.frames) {
        if (isWide) {
            frame.paddingLeftTop += QPoint(6, -10); // That's strange, but Battle sprites not perfectly centered. That's unconvenient.
        } else {
            frame.paddingLeftTop += QPoint(30, -10);
        }
    }
}

}

SpritePtr loadSpriteLegacy(const std_path& defFilePath)
{
    QFile defFile(stdPath2QString(defFilePath));
    if (!defFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    QByteArray defData = defFile.readAll();
    defFile.close();
    const std::string resourceName = path2string(defFilePath.filename().stem());

    DefParser cdefFile(defData.data(), defData.size(), getConfigForResource(resourceName), resourceName);
    if (!cdefFile.status())
        return {};

    SpriteLoader loader;
    for (const auto& parsedFrame : cdefFile.getFrames())
        loader.addFrame(parsedFrame.frameId, parsedFrame.data, parsedFrame.paddingLeftTop);
    for (const auto& parsedGroup : cdefFile.getGroups())
        loader.addGroup(parsedGroup.groupId, parsedGroup.frameIds, parsedGroup.boundarySize, SpriteSequenceParams{});

    return std::make_shared<Sprite>(std::move(loader));
}

SpritePtr loadPcx(const Mernel::std_path& pcxFilePath)
{
    QFile pcxFile(stdPath2QString(pcxFilePath));
    if (!pcxFile.open(QIODevice::ReadOnly)) {
        return {};
    }
    uint32_t fileSize = pcxFile.size();

    QDataStream ds(&pcxFile);
    ds.setByteOrder(QDataStream::LittleEndian);
    uint32_t size, width, height;
    ds >> size >> width >> height;

    if (size == 0x46323350 && width == 0 && height == 32) // 'P32F'
    {
        uint32_t u1, u2, u3, u4;
        ds >> size >> u1 >> u2 >> width >> height >> u3 >> u4;
        QImage tmp(width, height, QImage::Format_RGBA8888);
        for (uint32_t h = 0; h < height; h++)
            for (uint32_t w = 0; w < width; w++) {
                uint8_t r, g, b, a;
                ds >> b >> g >> r >> a;
                tmp.setPixelColor(w, height - h - 1, qRgba(r, g, b, a));
            }

        return Sprite::fromPixmap(QPixmap::fromImage(tmp));
    }
    if (width == 0 || height == 0) {
        return {};
    }

    if (width * height * 3 == size && fileSize == size + 12) { // RGB no pallete
        QImage tmp(width, height, QImage::Format_RGB888);
        for (uint32_t h = 0; h < height; h++)
            for (uint32_t w = 0; w < width; w++) {
                uint8_t r, g, b;
                ds >> b >> g >> r;
                tmp.setPixelColor(w, height - h - 1, qRgb(r, g, b));
            }

        return Sprite::fromPixmap(QPixmap::fromImage(tmp));
    }

    // PAL8 format. Pallete in the end of file.
    QVector<QVector<uint8_t>> pal8(height);

    QImage tmp(width, height, QImage::Format_RGB888);
    for (uint32_t h = 0; h < height; h++) {
        pal8[h].resize(width);
        for (uint32_t w = 0; w < width; w++) {
            ds >> pal8[h][w];
        }
    }
    QVector<QColor> palette(256);
    for (int i = 0; i < palette.size(); i++) {
        uint8_t r, g, b;
        ds >> r >> g >> b;
        palette[i] = qRgb(r, g, b);
    }
    for (uint32_t h = 0; h < height; h++) {
        for (uint32_t w = 0; w < width; w++)
            tmp.setPixelColor(w, h, palette[pal8[h][w]]);
    }
    return Sprite::fromPixmap(QPixmap::fromImage(tmp));
}

SpritePtr loadBmp(const std_path& bmpFilePath)
{
    QString path = stdPath2QString(bmpFilePath);
    QImage  img(path);
    return Sprite::fromPixmap(QPixmap::fromImage(img));
}

std::vector<int> paramsToInt(const std::vector<std::string>& params)
{
    std::vector<int> res(params.size());
    for (size_t i = 0; i < params.size(); ++i)
        res[i] = std::atoi(params[i].c_str());
    return res;
}

SpritePtr postProcessSprite(const Mernel::std_path& spritePath, SpritePtr sprite, const PropertyTreeMap& params, Core::IResourceLibrary* library)
{
    std::shared_ptr<SpriteLoader> result           = std::make_shared<SpriteLoader>();
    bool                          optimizeBoundary = params.contains("optimize_boundary");
    bool                          doTranspose      = params.contains("transpose");

    UnpackTaskList unpackTasks;
    if (params.contains("unpack")) {
        for (const auto& task : params.at("unpack").getList()) {
            const std::string tpl    = task["template"].getScalar().toString();
            const std::string subdir = task["subdir"].getScalar().toString();

            int idOffset   = task["idOffset"].getScalar().toInt();
            int startFrame = task["startFrame"].getScalar().toInt();
            int endFrame   = task["endFrame"].getScalar().toInt();
            unpackTasks.push_back({ tpl, subdir, idOffset, startFrame, endFrame });
        }
    }

    int maxWidth         = 0;
    int maxHeigth        = 0;
    int minPaddingWidth  = 100000;
    int minPaddingHeight = 100000;
    for (int g : sprite->getGroupsIds()) {
        auto oldSeqPtr = sprite->getFramesForGroup(g);
        auto newSeqPtr = std::make_shared<SpriteSequence>(*oldSeqPtr);
        for (const auto& [key, routineParam] : params) {
            if (key == "battle_unit") {
                postProcessSpriteGroupBattler(g, *newSeqPtr, routineParam);
            } else if (key == "make_transparent") {
                postProcessSpriteMakeTransparent(*newSeqPtr, routineParam);
            } else if (key == "flip_vertical") {
                postProcessSpriteFlipVertical(*newSeqPtr);
            }
        }
        if (optimizeBoundary) {
            for (auto& f : newSeqPtr->frames) {
                maxWidth         = std::max(maxWidth, f.frame.width() + f.paddingLeftTop.x());
                maxHeigth        = std::max(maxHeigth, f.frame.height() + f.paddingLeftTop.y());
                minPaddingWidth  = std::min(minPaddingWidth, f.paddingLeftTop.x());
                minPaddingHeight = std::min(minPaddingHeight, f.paddingLeftTop.y());
            }
            continue;
        }
        if (doTranspose) {
            int counter = 0;
            for (const auto& frame : oldSeqPtr->frames) {
                result->addFrame(counter, frame.frame, frame.paddingLeftTop);
                result->addGroup(counter,
                                 { counter },
                                 oldSeqPtr->boundarySize,
                                 oldSeqPtr->params);
                counter++;
            }

            continue;
        }
        for (const auto& utask : unpackTasks) {
            const QString tpl    = QString::fromStdString(utask.tpl);
            std::string   subdir = utask.subdir;
            if (!subdir.empty() && !subdir.ends_with('/'))
                subdir += '/';
            int idOffset   = utask.idOffset;
            int startFrame = utask.startFrame;
            int endFrame   = utask.endFrame;
            for (int f = startFrame, i = 0; f <= endFrame; ++f, ++i) {
                const int              id         = idOffset + i;
                const std::string      filename   = (tpl.contains("%1") ? tpl.arg(id) : tpl).toStdString();
                const std::string      filenameJs = filename + ".fhsprite.json";
                const Mernel::std_path path       = spritePath.parent_path() / Mernel::string2path(filenameJs);
                const SpriteFrame&     frame      = newSeqPtr->frames[f];

                {
                    auto spriteLoader = std::make_shared<SpriteLoader>();
                    spriteLoader->addFrame(0, frame.frame, frame.paddingLeftTop);
                    spriteLoader->addGroup(0, { 0 }, newSeqPtr->boundarySize, {});
                    saveSprite(spriteLoader, path, {});
                }
                {
                    /*ResourceMedia resource;
                    resource.type         = ResourceType::Sprite;
                    resource.id           = filename;
                    resource.mainFilename = filenameJs;
                    resource.subdir       = subdir;
                    library->registerResource(resource);*/
                }
            }
        }
        result->addGroupSeq(g, newSeqPtr);
    }
    if (optimizeBoundary) {
        for (int g : sprite->getGroupsIds()) {
            auto seqPtr    = sprite->getFramesForGroup(g);
            auto newSeqPtr = std::make_shared<SpriteSequence>(*seqPtr);

            newSeqPtr->boundarySize = QSize(maxWidth - minPaddingWidth, maxHeigth - minPaddingHeight);

            for (auto& f : newSeqPtr->frames) {
                f.paddingLeftTop -= QPoint(minPaddingWidth, minPaddingHeight);
            }
            result->addGroupSeq(g, newSeqPtr);
        }
    }

    return result;
}

void saveSpriteLegacy(Gui::SpritePtr sprite, const Mernel::std_path& defFilePath)
{
    ByteArrayHolder           m_binaryBuffer;
    ByteOrderBuffer           bobuffer(m_binaryBuffer);
    ByteOrderDataStreamWriter stream(bobuffer, ByteOrderDataStream::LITTLE_ENDIAN);

    auto seq = sprite->getFramesForGroup(0);

    Mernel::writeFileFromHolderThrow(defFilePath, m_binaryBuffer);
}

}
