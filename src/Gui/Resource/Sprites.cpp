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
#include "MernelPlatform/Profiler.hpp"

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
    Mernel::ProfilerScope scope1("Sprite::load");
    {
        std::string          buffer;
        Mernel::PropertyTree data;
        {
            Mernel::ProfilerScope scope2("read json file");
            buffer = Mernel::readFileIntoBuffer(jsonFilePath);
        }
        {
            Mernel::ProfilerScope scope2("parse json");
            data = Mernel::readJsonFromBuffer(buffer);
        }

        {
            Mernel::Reflection::PropertyTreeReader reader;
            Mernel::ProfilerScope                  scope2("deserialize from json");
            reader.jsonToValue(data, *this);
        }
    }
    {
        std::string buffer;
        {
            Mernel::ProfilerScope scope2("read image file");
            auto                  pngPath = makePngPath(jsonFilePath);
            buffer                        = Mernel::readFileIntoBuffer(pngPath);
        }
        {
            Mernel::ProfilerScope scope2("load pixmap");
            m_bitmap.loadFromData(reinterpret_cast<const uchar*>(buffer.data()), buffer.size());
        }
    }
}

void Sprite::save(const std_path& jsonFilePath) const
{
    Mernel::PropertyTree                   data;
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
    std::string buffer = Mernel::writeJsonToBuffer(data, true);
    Mernel::writeFileFromBuffer(jsonFilePath, buffer);

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

ISprite::SpriteSequencePtr Sprite::getFramesForGroup(int groupId) const
{
    if (!m_groups.contains(groupId))
        return nullptr;
    const Group& group = m_groups.at(groupId);
    if (group.m_cache)
        return group.m_cache;
    auto seq = std::make_shared<SpriteSequence>();

    seq->m_params = group.m_params;

    seq->m_boundarySize = m_boundarySize;
    seq->m_mask         = m_mask;

    for (const auto& frame : group.m_frames) {
        if (!frame.m_hasBitmap) {
            seq->m_frames << SpriteFrame{};
            continue;
        }

        auto framePix = m_bitmap.copy(frame.m_bitmapOffset.x(),
                                      frame.m_bitmapOffset.y(),
                                      frame.m_bitmapSize.width(),
                                      frame.m_bitmapSize.height());
        seq->m_frames << SpriteFrame{ .m_frame = std::move(framePix), .m_paddingLeftTop = frame.m_padding };
    }
    group.m_cache = seq;
    return seq;
}

}
