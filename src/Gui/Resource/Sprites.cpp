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

#include "SpritesReflection.hpp"

namespace FreeHeroes::Gui {
using namespace Mernel;
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
        ByteArrayHolder holder;
        {
            Mernel::ProfilerScope scope2("read image file");
            auto                  pngPath = makePngPath(jsonFilePath);
            holder                        = Mernel::readFileIntoHolder(pngPath);
        }
        {
            Mernel::ProfilerScope scope2("load pixmap");
            m_bitmap.loadPngFromBuffer(holder);
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
    auto                    pngPath = makePngPath(jsonFilePath);
    Mernel::ByteArrayHolder holder;
    m_bitmap.savePngToBuffer(holder);
    Mernel::writeFileFromHolder(pngPath, holder);
    if (!Mernel::std_fs::exists(pngPath)) {
        Mernel::std_fs::remove(jsonFilePath);
        throw std::runtime_error("failed to write:" + Mernel::path2string(pngPath));
    }
}

std::vector<int> Sprite::getGroupsIds() const
{
    std::vector<int> result;
    result.reserve(m_groups.size());
    for (const auto& [key, group] : m_groups)
        result.push_back(key);
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
            seq->m_frames.push_back(SpriteFrame{});
            continue;
        }

        auto framePix = m_bitmap.subframe(frame.m_bitmapOffset, frame.m_bitmapSize);
        seq->m_frames.push_back(SpriteFrame{ .m_frame = std::move(framePix), .m_paddingLeftTop = frame.m_padding });
    }
    group.m_cache = seq;
    return seq;
}

}
