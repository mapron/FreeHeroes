/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGraphicsLibrary.hpp"
#include "IResourceLibrary.hpp"

#include "GuiResourceExport.hpp"

namespace FreeHeroes::Gui {

class GUIRESOURCE_EXPORT GraphicsLibrary : public IGraphicsLibrary
{
public:
    GraphicsLibrary(Core::IResourceLibrary & resourceLibrary);
    ~GraphicsLibrary();

    IAsyncSpritePtr getObjectAnimation(const std::string & resourceName   ) override;
    IAsyncPixmapPtr getPixmapByKey    (const PixmapKey & resourceCode     ) override;
    IAsyncMoviePtr  getVideo          (const std::string& resourceName    ) override;
    IAsyncIconPtr   getIcon           (const PixmapKeyList & resourceCodes) override;

    PixmapKey splitKeyFromString(const std::string & resourceName) const override;
private:

private:
    class AsyncSprite;
    class AsyncPixmap;
    class AsyncMovie ;
    class AsyncIcon ;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
