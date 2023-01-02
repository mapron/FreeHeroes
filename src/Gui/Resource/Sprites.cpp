/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Sprites.hpp"

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"
#include "FsUtilsQt.hpp"
#include "MernelPlatform/StringUtils.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"

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

SpritePtr Sprite::fromPixmap(QPixmap pixmap)
{
    auto        seq = std::make_shared<SpriteSequence>();
    SpriteFrame f;
    f.frame          = pixmap;
    f.paddingLeftTop = { 0, 0 };
    seq->frames << f;

    seq->boundarySize = pixmap.size();

    Sprite result;
    result.addGroup(0, seq);
    return std::make_shared<Sprite>(std::move(result));
}

void Sprite::addGroup(int groupId, SpriteSequencePtr seq)
{
    m_groups[groupId] = seq;
}

void SpriteNew::load(const std_path& jsonFilePath)
{
    const std::string          buffer = Mernel::readFileIntoBufferThrow(jsonFilePath);
    const Mernel::PropertyTree data   = Mernel::readJsonFromBufferThrow(buffer);

    Mernel::Reflection::PropertyTreeReader reader;
    reader.jsonToValue(data, *this);

    m_bitmap.load(stdPath2QString(makePngPath(jsonFilePath)));
}

void SpriteNew::save(const std_path& jsonFilePath) const
{
    Mernel::PropertyTree                   data;
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
    std::string buffer = Mernel::writeJsonToBufferThrow(data, true);
    Mernel::writeFileFromBufferThrow(jsonFilePath, buffer);

    m_bitmap.save(stdPath2QString(makePngPath(jsonFilePath)));
}

}
