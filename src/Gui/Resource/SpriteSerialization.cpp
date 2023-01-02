/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpriteSerialization.hpp"

#include <QPainter>
#include <QDebug>

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"

#include <fstream>

namespace FreeHeroes::Gui {
using namespace Mernel;

namespace {
const std::string spriteJsonExtension{ ".fh.json" };
const std::string imageExtension{ ".png" };
}

SpriteFrame SpriteLoader::getSFrame(int id)
{
    SpriteFrame f;
    f.duplicate      = m_utilizedIds.contains(id);
    f.frame          = m_frames[id].first;
    f.paddingLeftTop = m_frames[id].second;
    f.id             = id;
    m_utilizedIds << id;
    return f;
}

void SpriteLoader::addFrame(int id, const QPixmap& data, const QPoint& padding)
{
    m_frames[id] = { data, padding };
}

void SpriteLoader::addGroup(int groupId, const QList<int>& frameIds, const QSize& boundarySize, const SpriteSequenceParams& params)
{
    auto seq = std::make_shared<SpriteSequence>();
    for (int id : frameIds)
        seq->frames << getSFrame(id);

    seq->params       = params;
    seq->boundarySize = boundarySize;

    addGroup(groupId, seq);
}

void SpriteLoader::addGroupSeq(int groupId, std::shared_ptr<SpriteSequence> seq)
{
    addGroup(groupId, seq);
}

bool saveSprite(const SpritePtr& spriteSet, const std_path& jsonFilePath, const SpriteSaveOptions& options)
{
    const std_path    folder       = jsonFilePath.parent_path();
    const std::string resourceName = Mernel::path2string(jsonFilePath.filename().stem().stem());

    if (!std_fs::exists(folder))
        std_fs::create_directories(folder);

    int totalWidth  = 0;
    int totalHeight = 0;

    //qWarning() << "saveSprite=" << resourceName.c_str() << "(folder=" << Mernel::path2string(folder).c_str();

    PropertyTree       groupsMap;
    QMap<int, QPixmap> framesMap;
    QMap<int, int>     groupIdToHeight;

    for (int groupId : spriteSet->getGroupsIds()) {
        const SpriteSequencePtr pg = spriteSet->getFramesForGroup(groupId);

        const int boundarySizeWidth  = pg->boundarySize.width();
        const int boundarySizeHeight = pg->boundarySize.height();
        int       maxFrameHeight     = 0;
        int       totalFrameWidth    = 0;

        PropertyTree group;
        group["boundarySizeWidth"]  = PropertyTreeScalar(boundarySizeWidth);
        group["boundarySizeHeight"] = PropertyTreeScalar(boundarySizeHeight);
        {
            PropertyTree&               extraObj = group["extra"];
            const SpriteSequenceParams& p        = pg->params;
            if (p.scaleFactorPercent != 100)
                extraObj["scaleFactorPercent"] = PropertyTreeScalar(p.scaleFactorPercent);
            if (p.animationCycleDuration != 1000)
                extraObj["animationCycleDuration"] = PropertyTreeScalar(p.animationCycleDuration);
            if (p.specialFrameIndex != -1)
                extraObj["specialFrameIndex"] = PropertyTreeScalar(p.specialFrameIndex);
            if (!p.actionPoint.isNull()) {
                extraObj["actionPointX"] = PropertyTreeScalar(p.actionPoint.x());
                extraObj["actionPointY"] = PropertyTreeScalar(p.actionPoint.y());
            }
        }

        group["pixHeightOffset"] = PropertyTreeScalar(totalHeight);
        PropertyTree framesJson;
        for (const SpriteFrame& frame : pg->frames) {
            PropertyTree frameJson;
            const bool   dup     = frame.duplicate && options.removeDuplicateFrames;
            frameJson["id"]      = PropertyTreeScalar(frame.id);
            frameJson["dup"]     = PropertyTreeScalar(dup);
            frameJson["padLeft"] = PropertyTreeScalar(frame.paddingLeftTop.x());
            frameJson["padTop"]  = PropertyTreeScalar(frame.paddingLeftTop.y());
            if (!dup && !options.splitIntoPngFiles) {
                frameJson["w"] = PropertyTreeScalar(frame.frame.width());
                frameJson["h"] = PropertyTreeScalar(frame.frame.height());
            }
            framesJson.append(frameJson);
            if (dup)
                continue;

            framesMap[frame.id] = frame.frame;
            maxFrameHeight      = std::max(maxFrameHeight, frame.frame.height());
            totalFrameWidth += frame.frame.width();
        }
        group["frames"]                    = framesJson;
        groupsMap[std::to_string(groupId)] = group;

        totalWidth = std::max(totalFrameWidth, totalWidth);
        totalHeight += maxFrameHeight;
        groupIdToHeight[groupId] = maxFrameHeight;
    }
    QPixmap out;
    if (!options.splitIntoPngFiles) {
        out = QPixmap(QSize(totalWidth, totalHeight));
        out.fill(QColor(0, 0, 0, 0));
    }
    int x = 0;
    int y = 0;
    //int savedFrames = 0, skippedFrames = 0;
    for (int groupId : spriteSet->getGroupsIds()) {
        const SpriteSequencePtr pg     = spriteSet->getFramesForGroup(groupId);
        const int               height = groupIdToHeight[groupId];
        if (height == 0)
            continue;

        for (const SpriteFrame& frame : pg->frames) {
            if (frame.duplicate && options.removeDuplicateFrames)
                continue;

            if (options.splitIntoPngFiles) {
                QImage img = frame.frame.toImage();

                auto subfolder = folder / resourceName;
                if (!std::filesystem::exists(subfolder))
                    std::filesystem::create_directories(subfolder);
                auto outImagePath = subfolder / (std::to_string(frame.id) + imageExtension);
                if (!img.save(stdPath2QString(outImagePath)))
                    return false;

            } else if (!frame.frame.isNull()) {
                QPainter painter(&out);
                painter.drawPixmap(QPoint(x, y), frame.frame);
            }
            x += frame.frame.width();
        }
        y += height;
        x = 0;
    }
    if (!options.splitIntoPngFiles) {
        QImage img          = out.toImage();
        auto   outImagePath = folder / (std_path(resourceName).concat(imageExtension));
        if (!img.save(stdPath2QString(outImagePath)))
            return false;
    }

    PropertyTree root;
    std::string  buffer;

    root["version"]       = PropertyTreeScalar("1.0");
    root["groups"]        = std::move(groupsMap);
    root["splitToFolder"] = PropertyTreeScalar(options.splitIntoPngFiles);

    return Mernel::writeJsonToBuffer(buffer, root, true) && Mernel::writeFileFromBuffer(jsonFilePath, buffer);
}

namespace {

struct GroupOffsetList {
    QList<int>           frameIds;
    int                  groupId;
    QSize                boundarySize;
    SpriteSequenceParams extra;
};

}

SpritePtr loadSprite(const std_path& jsonFilePath)
{
    const std_path    folder       = jsonFilePath.parent_path();
    const std::string resourceName = Mernel::path2string(jsonFilePath.filename().stem().stem());

    //qWarning() << "loadSprite=" << resourceName.c_str() << "(folder=" << Mernel::path2string(folder).c_str();

    std::string buffer;
    if (!Mernel::readFileIntoBuffer(jsonFilePath, buffer))
        return {};

    PropertyTree root;
    if (!Mernel::readJsonFromBuffer(buffer, root))
        return {};

    QMap<int, QPair<QPixmap, QPoint>> pixes;

    const auto& groupsObjList = root["groups"];
    const bool  usePngSplit   = root["splitToFolder"].getScalar().toBool();
    QPixmap     inPix;
    if (!usePngSplit) {
        auto   pngFilename = folder / (std_path(resourceName).concat(imageExtension));
        QImage inData(stdPath2QString(pngFilename));
        inPix = QPixmap::fromImage(inData);
    }

    QVector<GroupOffsetList> allTasks;

    for (const auto& [groupsObjKey, groupsObj] : groupsObjList.getMap()) {
        const int   groupId     = std::atoi(groupsObjKey.c_str());
        const auto& framesArray = groupsObj["frames"];

        GroupOffsetList groupTasks;

        const int pixHeightOffset    = groupsObj.contains("pixHeightOffset") ? groupsObj["pixHeightOffset"].getScalar().toInt() : 0;
        const int boundarySizeWidth  = groupsObj["boundarySizeWidth"].getScalar().toInt();
        const int boundarySizeHeight = groupsObj["boundarySizeHeight"].getScalar().toInt();
        groupTasks.boundarySize      = { boundarySizeWidth, boundarySizeHeight };
        if (groupsObj.contains("extra") && groupsObj["extra"].isMap()) {
            const auto& extraObj                    = groupsObj["extra"];
            groupTasks.extra.scaleFactorPercent     = extraObj.value("scaleFactorPercent", PropertyTreeScalar(100)).toInt();
            groupTasks.extra.specialFrameIndex      = extraObj.value("specialFrameIndex", PropertyTreeScalar(-1)).toInt();
            groupTasks.extra.animationCycleDuration = extraObj.value("animationCycleDuration", PropertyTreeScalar(1000)).toInt();

            groupTasks.extra.actionPoint = extraObj.contains("actionPointX") ? QPoint{ (int) extraObj.value("actionPointX", PropertyTreeScalar(0)).toInt(), (int) extraObj.value("actionPointY", PropertyTreeScalar(0)).toInt() } : QPoint{};
        }

        groupTasks.groupId = groupId;

        int xOffset = 0;
        for (const auto& frameObj : framesArray.getList()) {
            const int  id      = frameObj["id"].getScalar().toInt();
            const bool dup     = frameObj["dup"].getScalar().toInt();
            const int  padLeft = frameObj["padLeft"].getScalar().toInt();
            const int  padTop  = frameObj["padTop"].getScalar().toInt();
            const int  w       = frameObj.value("w", PropertyTreeScalar(0)).toInt();
            const int  h       = frameObj.value("h", PropertyTreeScalar(0)).toInt();

            groupTasks.frameIds << id;

            if (dup)
                continue;

            assert(!pixes.contains(id)); // dup error!

            QPixmap framePix;
            if (usePngSplit) {
                auto   pngFilename = folder / resourceName / (std::to_string(id) + imageExtension);
                QImage inData(stdPath2QString(pngFilename));
                framePix = QPixmap::fromImage(inData);
            } else if (w > 0 && h > 0) {
                framePix = inPix.copy(xOffset, pixHeightOffset, w, h);
            }
            xOffset += w;
            pixes[id] = QPair{ framePix, QPoint{ padLeft, padTop } };
        }
        allTasks.push_back(groupTasks);
    }
    SpriteLoader result;
    for (int id : pixes.keys()) {
        result.addFrame(id, pixes[id].first, pixes[id].second);
    }
    for (const GroupOffsetList& groupTasks : allTasks) {
        result.addGroup(groupTasks.groupId,
                        groupTasks.frameIds,
                        groupTasks.boundarySize,
                        groupTasks.extra);
    }

    return std::make_unique<Sprite>(std::move(result));
}

std::string makeJsonFilename(const std::string& resourceName)
{
    return resourceName + spriteJsonExtension;
}

}
