/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Sprites.hpp"

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"
#include "MernelPlatform/Logger.hpp"

#include "FsUtilsQt.hpp"
#include "SpritesReflection.hpp"

namespace FreeHeroes::Gui {

namespace {
std_path makePngPath(const std_path& jsonFilePath)
{
    auto baseName = jsonFilePath.filename().stem();
    if (baseName.has_extension())
        baseName = baseName.stem();
    return jsonFilePath.parent_path() / (baseName.concat(".png"));
}
}

void Sprite::load(const std_path& jsonFilePath)
{
    const std::string          buffer = Mernel::readFileIntoBufferThrow(jsonFilePath);
    const Mernel::PropertyTree data   = Mernel::readJsonFromBufferThrow(buffer);

    Mernel::Reflection::PropertyTreeReader reader;
    reader.jsonToValue(data, *this);

    m_bitmap.load(stdPath2QString(makePngPath(jsonFilePath)));
}

void Sprite::save(const std_path& jsonFilePath) const
{
    Mernel::PropertyTree                   data;
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
    std::string buffer = Mernel::writeJsonToBufferThrow(data, true);
    Mernel::writeFileFromBufferThrow(jsonFilePath, buffer);

    Mernel::std_fs::create_directories(jsonFilePath.parent_path());
    m_bitmap.save(stdPath2QString(makePngPath(jsonFilePath)));
}

QList<int> Sprite::getGroupsIds() const
{
    QList<int> result;
    for (const auto& [key, group] : m_groups)
        result << key;
    return result;
}

SpriteSequencePtr Sprite::getFramesForGroup(int groupId) const
{
    if (!m_groups.contains(groupId))
        return nullptr;
    const Group& group = m_groups.at(groupId);
    if (group.m_cache)
        return group.m_cache;
    auto seq = std::make_shared<SpriteSequence>();

    seq->params = {
        .scaleFactorPercent     = group.m_params.m_scaleFactorPercent,
        .animationCycleDuration = group.m_params.m_animationCycleDuration,
        .specialFrameIndex      = group.m_params.m_specialFrameIndex,
        .actionPoint            = group.m_params.m_actionPoint,
    };

    seq->boundarySize = m_boundarySize;

    for (const auto& frame : group.m_frames) {
        if (!frame.m_hasBitmap) {
            seq->frames << SpriteFrame{};
            continue;
        }

        auto framePix = m_bitmap.copy(frame.m_bitmapOffset.x(),
                                      frame.m_bitmapOffset.y(),
                                      frame.m_bitmapSize.width(),
                                      frame.m_bitmapSize.height());
        seq->frames << SpriteFrame{ .frame = std::move(framePix), .paddingLeftTop = frame.m_padding };
    }
    group.m_cache = seq;
    return seq;
}

}
