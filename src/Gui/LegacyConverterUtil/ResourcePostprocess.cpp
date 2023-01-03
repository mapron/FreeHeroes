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
QPixmap getPixmap(Core::IResourceLibrary& resources, std::string id)
{
    const auto resource = resources.get(ResourceType::Sprite, id);
    auto       sprite   = loadSprite(resource);
    Q_ASSERT(sprite && sprite->getGroupsCount() > 0);

    auto seq = sprite->getFramesForGroup(0);
    return seq->frames[0].frame;
}
}

void ResourcePostprocess::concatSprites(Core::IResourceLibrary&         resources,
                                        const std::vector<std::string>& in,
                                        const std::string&              out,
                                        bool                            vertical)
{
    if (resources.fileExists(ResourceType::Sprite, out))
        return;

    QList<QImage> imgs;
    QSize         size(0, 0);
    for (auto& resId : in) {
        auto img = getPixmap(resources, resId).toImage();
        imgs << img;
        if (vertical) {
            size.setWidth(std::max(size.width(), img.width()));
            size.setHeight(size.height() + img.height());
        } else {
            size.setWidth(size.width() + img.width());
            size.setHeight(std::max(size.height(), img.height()));
        }
    }

    QImage result(size.width(), size.height(), QImage::Format_RGBA8888);
    QPoint offset(0, 0);
    for (auto& img : imgs) {
        QPainter p(&result);
        p.drawImage(offset, img);

        if (vertical) {
            offset += QPoint(0, img.height());
        } else {
            offset += QPoint(img.width(), 0);
        }
    }

    // @todo: need to know where to save?
    // SpritePtr sprite = Sprite::fromPixmap(QPixmap::fromImage(result));
    // saveSprite(sprite, std_path(resource.getFullPath()));
}

void ResourcePostprocess::concatTilesSprite(Core::IResourceLibrary& resources, const std::string& base, const std::string& out, int tilesCount)
{
    auto makeId = [base](int i) {
        auto idSuffix = std::to_string(i);
        while (idSuffix.size() < 3)
            idSuffix = "0" + idSuffix;
        return base + idSuffix;
    };
    // @todo:
    /*
    auto resource         = resources.get(ResourceType::Sprite, makeId(0));
    resource.id           = out;
    auto str              = makeJsonFilename(out);
    resource.subdir       = "Sprites/AdventureTiles/";
    resource.mainFilename = str; // string_view, cannot make temporary here!
    if (resources.mediaExists(ResourceType::Sprite, out))
        return;

    resources.registerResource(resource);

    SpriteLoader result;
    for (int i = 0; i < tilesCount; ++i) {
        QPixmap pix = getPixmap(resources, makeId(i));
        result.addFrame(i, pix, {});
        result.addGroup(i, { i }, pix.size(), {});
    }
    auto sharedSprite = std::make_shared<const Sprite>(std::move(result));

    const auto& path = std_path(resource.getFullPath());
    saveSprite(sharedSprite, path, {});*/
}

}
