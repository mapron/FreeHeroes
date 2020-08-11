/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ResourcePostprocess.hpp"

#include "Sprites.hpp"
#include "SpriteSerialization.hpp"

#include <QPainter>

namespace FreeHeroes::Conversion {
using namespace Core;
using namespace Gui;
namespace {
QImage getImage(Core::IResourceLibrary& resources, std::string id) {
    const auto & resource = resources.getMedia(ResourceMedia::Type::Sprite, id);
    auto sprite = loadSprite(std_path(resource.getFullPath()));
    Q_ASSERT(sprite && sprite->getGroupsCount() > 0);

    auto seq = sprite->getFramesForGroup(0);
    return seq->frames[0].frame.toImage();
}
}

void ResourcePostprocess::concatSprites(Core::IResourceLibrary& resources,
                                       const std::vector<std::string>& in,
                                       const std::string& out,
                                       bool vertical)
{
    auto resource = resources.getMedia(ResourceMedia::Type::Sprite, in[0]);
    resource.id = out;
    auto str = makeJsonFilename(out);
    resource.mainFilename = str; // string_view, cannon make temporary here!
    if (resources.mediaExists(ResourceMedia::Type::Sprite, out))
        return;

    resources.registerResource(resource);

    QList<QImage> imgs;
    QSize size(0,0);
    for (auto & resId: in) {
        auto img = getImage(resources, resId);
        imgs << img;
        if (vertical) {
            size.setWidth(std::max(size.width(), img.width()));
            size.setHeight(size.height()  +  img.height());
        } else {
            size.setWidth(size.width()  +  img.width());
            size.setHeight(std::max(size.height(), img.height()));
        }
    }

    QImage result(size.width(), size.height(), QImage::Format_RGBA8888);
    QPoint offset(0, 0);
    for (auto & img : imgs) {
        QPainter p(&result);
        p.drawImage(offset, img);

        if (vertical) {
            offset += QPoint(0, img.height());
        } else {
            offset += QPoint(img.width(), 0);
        }
    }

    SpritePtr sprite = Sprite::fromPixmap(QPixmap::fromImage(result));
    saveSprite(sprite, std_path(resource.getFullPath()));
}


}
