/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GraphicsLibrary.hpp"

#include "SpriteSerialization.hpp"

#include "Logger.hpp"

#include <QBitmap>
#include <QPainter>
#include <QSet>
#include <QIcon>
#include <QMovie>

template<>
bool qMapLessThanKey<FreeHeroes::Gui::IGraphicsLibrary::PixmapKey>(const FreeHeroes::Gui::IGraphicsLibrary::PixmapKey& key1, const FreeHeroes::Gui::IGraphicsLibrary::PixmapKey& key2)
{
    return std::tie(key1.resourceName, key1.group, key1.frame) < std::tie(key2.resourceName, key2.group, key2.frame);
}

template<>
bool qMapLessThanKey<FreeHeroes::Gui::IGraphicsLibrary::PixmapKeyList>(const FreeHeroes::Gui::IGraphicsLibrary::PixmapKeyList& key1, const FreeHeroes::Gui::IGraphicsLibrary::PixmapKeyList& key2)
{
    const bool isRangeLess = std::lexicographical_compare(key1.cbegin(), key1.cend(), key2.cbegin(), key2.cend(), [](const auto& l, const auto& r) { return qMapLessThanKey(l, r); });
    return isRangeLess;
}

namespace FreeHeroes::Gui {
using namespace Core;

class GraphicsLibrary::AsyncSprite : public IAsyncSprite {
public:
    GraphicsLibrary::Impl* m_impl;
    std::string            m_resourceName;
    AsyncSprite(GraphicsLibrary::Impl* impl, const std::string& resourceName)
        : m_impl(impl)
        , m_resourceName(resourceName)
    {}

    bool      exists() const override;
    bool      isLoaded() const override;
    bool      preload() const override;
    SpritePtr get() const override;
};
class GraphicsLibrary::AsyncPixmap : public IAsyncPixmap {
public:
    GraphicsLibrary::Impl* m_impl;
    const PixmapKey        m_resourceCode;
    AsyncPixmap(GraphicsLibrary::Impl* impl, const PixmapKey& resourceCode)
        : m_impl(impl)
        , m_resourceCode(resourceCode)
    {}

    bool    exists() const override;
    bool    isLoaded() const override;
    bool    preload() const override;
    QPixmap get() const override;
};
class GraphicsLibrary::AsyncMovie : public IAsyncMovie {
public:
    GraphicsLibrary::Impl* m_impl;
    std::string            m_resourceName;
    AsyncMovie(GraphicsLibrary::Impl* impl, const std::string& resourceName)
        : m_impl(impl)
        , m_resourceName(resourceName)
    {}

    bool    exists() const override;
    bool    isLoaded() const override;
    bool    preload() const override;
    QMovie* create(QObject* parent) const override;
};
class GraphicsLibrary::AsyncIcon : public IAsyncIcon {
public:
    GraphicsLibrary::Impl* m_impl;
    const PixmapKeyList    m_resourceCodes;
    AsyncIcon(GraphicsLibrary::Impl* impl, const PixmapKeyList& resourceCodes)
        : m_impl(impl)
        , m_resourceCodes(resourceCodes)
    {}

    bool  exists() const override;
    bool  isLoaded() const override;
    bool  preload() const override;
    QIcon get() const override;
};

struct GraphicsLibrary::Impl {
    const Core::IResourceLibrary* m_resourceLibrary;
    QMap<std::string, SpritePtr>  m_spriteCache;
    QMap<std::string, QImage>     m_imageCache;
    QMap<PixmapKey, QPixmap>      m_pixmapCache;
    QMap<PixmapKeyList, QIcon>    m_iconCache;

    SpritePtr getSyncObjectAnimation(const std::string& resourceName);
    QPixmap   getSyncPixmapByKey(const PixmapKey& resourceCode);
    QMovie*   getSyncVideo(const std::string& resourceName, QObject* parent);
    QIcon     getSyncIcon(const PixmapKeyList& resourceCodes);

    bool isSpriteResourceExists(const std::string& resourceName) const
    {
        return m_resourceLibrary->mediaExists(ResourceMedia::Type::Sprite, resourceName);
    }
    bool isVideoResourceExists(const std::string& resourceName)
    {
        return m_resourceLibrary->mediaExists(ResourceMedia::Type::Video, resourceName);
    }

    Impl(const Core::IResourceLibrary* resourceLibrary)
        : m_resourceLibrary(resourceLibrary)
    {
    }
};

GraphicsLibrary::GraphicsLibrary(const Core::IResourceLibrary* resourceLibrary)
    : m_impl(std::make_unique<Impl>(resourceLibrary))
{
}

GraphicsLibrary::~GraphicsLibrary()
{
}

IAsyncSpritePtr GraphicsLibrary::getObjectAnimation(const std::string& resourceName)
{
    return std::make_shared<AsyncSprite>(m_impl.get(), resourceName);
}

IAsyncPixmapPtr GraphicsLibrary::getPixmapByKey(const PixmapKey& resourceCode)
{
    return std::make_shared<AsyncPixmap>(m_impl.get(), resourceCode);
}

IAsyncMoviePtr GraphicsLibrary::getVideo(const std::string& resourceName)
{
    return std::make_shared<AsyncMovie>(m_impl.get(), resourceName);
}

IAsyncIconPtr GraphicsLibrary::getIcon(const PixmapKeyList& resourceCodes)
{
    return std::make_shared<AsyncIcon>(m_impl.get(), resourceCodes);
}

IGraphicsLibrary::PixmapKey GraphicsLibrary::splitKeyFromString(const std::string& resourceName) const
{
    QStringList parts      = QString::fromStdString(resourceName).split(":");
    std::string resourceId = parts.takeFirst().toStdString();
    int         groupId    = 0;
    int         frameId    = 0;
    if (parts.size() == 2) {
        groupId = parts[0].toInt();
        frameId = parts[1].toInt();
    } else if (parts.size() == 1) {
        frameId = parts[0].toInt();
    }

    return PixmapKey{ resourceId, groupId, frameId };
}

SpritePtr GraphicsLibrary::Impl::getSyncObjectAnimation(const std::string& resourceName)
{
    if (m_spriteCache.contains(resourceName)) {
        return m_spriteCache[resourceName];
    }
    auto& sprite = m_spriteCache[resourceName];
    if (!m_resourceLibrary->mediaExists(ResourceMedia::Type::Sprite, resourceName))
        return sprite;

    const auto& resource = m_resourceLibrary->getMedia(ResourceMedia::Type::Sprite, resourceName);
    sprite               = loadSprite(resource.getFullPath());
    Q_ASSERT(sprite && sprite->getGroupsCount() > 0);
    if (sprite->getGroupsCount() == 0)
        sprite = nullptr;

    return sprite;
}

QPixmap GraphicsLibrary::Impl::getSyncPixmapByKey(const PixmapKey& resourceCode)
{
    if (m_pixmapCache.contains(resourceCode)) {
        return m_pixmapCache[resourceCode];
    }
    QPixmap&  result = m_pixmapCache[resourceCode];
    SpritePtr sprite = getSyncObjectAnimation(resourceCode.resourceName);
    if (!sprite)
        return result;

    if (!sprite->getGroupsIds().contains(resourceCode.group))
        return result;

    auto seq = sprite->getFramesForGroup(resourceCode.group);
    if (resourceCode.frame >= seq->frames.size() || resourceCode.frame < 0) // @todo: maybe allow Python-like [-1] = last elem etc.
        return result;

    result = seq->frames[resourceCode.frame].frame;
    if (seq->boundarySize != result.size()) {
        QImage padded{ seq->boundarySize, QImage::Format_RGBA8888 };
        padded.fill(Qt::transparent);
        QPainter p{ &padded };
        p.drawPixmap(seq->frames[resourceCode.frame].paddingLeftTop, result);
        result = QPixmap::fromImage(padded);
    }
    return result;
}

QMovie* GraphicsLibrary::Impl::getSyncVideo(const std::string& resourceName, QObject* parent)
{
    const auto& resource = m_resourceLibrary->getMedia(ResourceMedia::Type::Video, resourceName);
    QString     path     = stdPath2QString(resource.getFullPath());
    QMovie*     movie    = new QMovie(path, QByteArray(), parent);
    return movie;
}

QIcon GraphicsLibrary::Impl::getSyncIcon(const PixmapKeyList& resourceCodes)
{
    if (m_iconCache.contains(resourceCodes)) {
        return m_iconCache[resourceCodes];
    }
    QIcon& icon = m_iconCache[resourceCodes];
    // normal, pressed, disabled, hovered.
    // normal    = QIcon::Off +  QIcon::Normal
    // pressed   = QIcon::On  +  QIcon::Normal
    // disabled  = QIcon::Off +  QIcon::Disabled
    // hovered   = QIcon::Off +  QIcon::Selected
    QList<QPixmap> pixmaps;
    for (auto& resourceId : resourceCodes)
        pixmaps << (resourceId.resourceName.empty() ? QPixmap() : getSyncPixmapByKey(resourceId));
    icon = QIcon(pixmaps[0]);
    if (pixmaps.size() > 1 && !pixmaps[1].isNull()) {
        icon.addPixmap(pixmaps[1], QIcon::Normal, QIcon::On);
    }
    if (pixmaps.size() > 2 && !pixmaps[2].isNull()) {
        icon.addPixmap(pixmaps[2], QIcon::Disabled, QIcon::Off);
    }
    if (pixmaps.size() > 3 && !pixmaps[3].isNull()) {
        icon.addPixmap(pixmaps[3], QIcon::Selected, QIcon::Off);
    }
    return icon;
}

bool GraphicsLibrary::AsyncSprite::exists() const
{
    return m_impl->isSpriteResourceExists(m_resourceName);
}

bool GraphicsLibrary::AsyncSprite::isLoaded() const
{
    return m_impl->m_spriteCache.contains(m_resourceName);
}

bool GraphicsLibrary::AsyncSprite::preload() const
{
    return m_impl->getSyncObjectAnimation(m_resourceName) != nullptr;
}

SpritePtr GraphicsLibrary::AsyncSprite::get() const
{
    return m_impl->getSyncObjectAnimation(m_resourceName);
}

bool GraphicsLibrary::AsyncPixmap::exists() const
{
    return m_impl->isSpriteResourceExists(m_resourceCode.resourceName);
}

bool GraphicsLibrary::AsyncPixmap::isLoaded() const
{
    return m_impl->m_pixmapCache.contains(m_resourceCode);
}

bool GraphicsLibrary::AsyncPixmap::preload() const
{
    return !m_impl->getSyncPixmapByKey(m_resourceCode).isNull();
}

QPixmap GraphicsLibrary::AsyncPixmap::get() const
{
    return m_impl->getSyncPixmapByKey(m_resourceCode);
}

bool GraphicsLibrary::AsyncMovie::exists() const
{
    return m_impl->isVideoResourceExists(m_resourceName);
}

bool GraphicsLibrary::AsyncMovie::isLoaded() const
{
    return true;
}

bool GraphicsLibrary::AsyncMovie::preload() const
{
    return true;
}

QMovie* GraphicsLibrary::AsyncMovie::create(QObject* parent) const
{
    return m_impl->getSyncVideo(m_resourceName, parent);
}

bool GraphicsLibrary::AsyncIcon::exists() const
{
    for (const auto& code : m_resourceCodes)
        if (!m_impl->isSpriteResourceExists(code.resourceName))
            return false;
    return true;
}

bool GraphicsLibrary::AsyncIcon::isLoaded() const
{
    return m_impl->m_iconCache.contains(m_resourceCodes);
}

bool GraphicsLibrary::AsyncIcon::preload() const
{
    return !m_impl->getSyncIcon(m_resourceCodes).isNull();
}

QIcon GraphicsLibrary::AsyncIcon::get() const
{
    return m_impl->getSyncIcon(m_resourceCodes);
}

}
