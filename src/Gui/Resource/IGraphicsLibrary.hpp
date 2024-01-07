/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGuiResource.hpp"

namespace FreeHeroes::Gui {

class IGraphicsLibrary {
public:
    virtual ~IGraphicsLibrary() = default;

    struct PixmapKey {
        std::string resourceName;
        int         group = 0;
        int         frame = 0;
        PixmapKey()       = default;
        PixmapKey(const std::string& resource)
            : resourceName(resource)
        {}
        PixmapKey(const std::string& resource, int frame)
            : resourceName(resource)
            , group(0)
            , frame(frame)
        {}
        PixmapKey(const std::string& resource, int group, int frame)
            : resourceName(resource)
            , group(group)
            , frame(frame)
        {}

        auto operator<=>(const PixmapKey&) const noexcept = default;
    };
    using PixmapKeyList = std::vector<PixmapKey>;

    virtual IAsyncSpritePtr getObjectAnimation(const std::string& resourceName) const = 0;
    virtual IAsyncPixmapPtr getPixmapByKey(const PixmapKey& resourceCode) const       = 0;
    virtual IAsyncMoviePtr  getVideo(const std::string& resourceName) const           = 0;
    virtual IAsyncIconPtr   getIcon(const PixmapKeyList& resourceCodes) const         = 0;

    virtual PixmapKey splitKeyFromString(const std::string& resourceName) const = 0;

    IAsyncPixmapPtr getPixmap(const std::string& resourceCode) const
    {
        auto key = splitKeyFromString(resourceCode);
        return getPixmapByKey(key);
    }
    IAsyncIconPtr getIconFromCodeList(std::vector<std::string> resourceCodes) const
    {
        PixmapKeyList tmp;
        tmp.reserve(resourceCodes.size());
        for (auto& s : resourceCodes)
            tmp.emplace_back(std::move(s), 0, 0);
        return getIcon(tmp);
    }
    IAsyncIconPtr getIcon(const std::string& resourceCodeBase, int count) const
    {
        PixmapKeyList tmp;
        for (int i = 0; i < count; ++i)
            tmp.emplace_back(resourceCodeBase, 0, i);
        return getIcon(tmp);
    }
};
using IGraphicsLibraryPtr = std::shared_ptr<IGraphicsLibrary>;

}
