/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGraphicsLibrary.hpp"
#include "IResourceLibrary.hpp"

#include "GuiResourceExport.hpp"

namespace FreeHeroes::Gui {

class GUIRESOURCE_EXPORT GraphicsLibrary : public IGraphicsLibrary {
public:
    GraphicsLibrary(const Core::IResourceLibrary* resourceLibrary);
    ~GraphicsLibrary();

    IAsyncSpritePtr getObjectAnimation(const std::string& resourceName) const override;
    IAsyncPixmapPtr getPixmapByKey(const PixmapKey& resourceCode) const override;
    IAsyncMoviePtr  getVideo(const std::string& resourceName) const override;
    IAsyncIconPtr   getIcon(const PixmapKeyList& resourceCodes) const override;

    PixmapKey splitKeyFromString(const std::string& resourceName) const override;

private:
    class AsyncSprite;
    class AsyncPixmap;
    class AsyncMovie;
    class AsyncIcon;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
