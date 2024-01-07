/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ISprites.hpp"

#include <memory>

class QIcon;
class QMovie;
class QObject;
class QPixmap;

namespace FreeHeroes::Gui {

class IGuiAsyncResource {
public:
    virtual ~IGuiAsyncResource() = default;

    virtual bool exists() const   = 0;
    virtual bool isLoaded() const = 0;
    virtual bool preload() const  = 0;
};

class IAsyncSprite : public IGuiAsyncResource {
public:
    virtual SpritePtr get() const = 0;
};
using IAsyncSpritePtr = std::shared_ptr<const IAsyncSprite>;

class IAsyncPixmap : public IGuiAsyncResource {
public:
    virtual QPixmap get() const = 0;
};
using IAsyncPixmapPtr = std::shared_ptr<const IAsyncPixmap>;

class IAsyncIcon : public IGuiAsyncResource {
public:
    virtual QIcon get() const = 0;
};
using IAsyncIconPtr = std::shared_ptr<const IAsyncIcon>;

class IAsyncMovie : public IGuiAsyncResource {
public:
    virtual QMovie* create(QObject* parent) const = 0;
};
using IAsyncMoviePtr = std::shared_ptr<const IAsyncMovie>;

}
