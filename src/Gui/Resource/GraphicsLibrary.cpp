/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GraphicsLibrary.hpp"

#include "Sprites.hpp"

#include "FsUtilsQt.hpp"

#include "MernelPlatform/Logger.hpp"

#include <QBitmap>
#include <QPainter>
#include <QSet>
#include <QIcon>
#include <QMovie>

namespace FreeHeroes::Gui {
using namespace Core;

template<class Key, class Object>
class CacheContainer {
public:
    struct AsyncRecord {
        Key             m_key;
        CacheContainer* m_container = nullptr;
        Object          m_object;
        bool            m_exists     = false;
        bool            m_loadCached = false;
        bool            m_loadResult = false;

        bool exists() { return m_exists; }
        bool isLoaded() { return m_loadCached && m_loadResult; }
        bool preload()
        {
            if (m_loadCached)
                return m_loadResult;
            m_loadResult = m_container->m_factory(m_key, m_object);
            m_loadCached = true;
            return m_loadResult;
        }
        Object get()
        {
            preload();
            return m_object;
        }
    };

    AsyncRecord& makeAsyncRecord(const Key& key)
    {
        auto it = m_records.find(key);
        if (it != m_records.cend())
            return it->second;

        AsyncRecord& result = m_records[key];
        result.m_key        = key;
        result.m_container  = this;
        result.m_exists     = m_existCheck(key);

        return result;
    }

    std::function<bool(const Key&)>          m_existCheck;
    std::function<bool(const Key&, Object&)> m_factory;
    std::map<Key, AsyncRecord>               m_records;
};

using SpriteContainer = CacheContainer<std::string, SpritePtr>;
using PixmapContainer = CacheContainer<IGraphicsLibrary::PixmapKey, QPixmap>;
using IconContainer   = CacheContainer<IGraphicsLibrary::PixmapKeyList, QIcon>;

class GraphicsLibrary::AsyncSprite : public IAsyncSprite {
public:
    SpriteContainer::AsyncRecord& m_record;

    AsyncSprite(GraphicsLibrary::Impl* impl, const std::string& resourceName);

    bool      exists() const override { return m_record.exists(); }
    bool      isLoaded() const override { return m_record.isLoaded(); }
    bool      preload() const override { return m_record.preload(); }
    SpritePtr get() const override { return m_record.get(); }
};
class GraphicsLibrary::AsyncPixmap : public IAsyncPixmap {
public:
    PixmapContainer::AsyncRecord& m_record;
    AsyncPixmap(GraphicsLibrary::Impl* impl, const PixmapKey& resourceCode);

    bool    exists() const override { return m_record.exists(); }
    bool    isLoaded() const override { return m_record.isLoaded(); }
    bool    preload() const override { return m_record.preload(); }
    QPixmap get() const override { return m_record.get(); }
};
class GraphicsLibrary::AsyncIcon : public IAsyncIcon {
public:
    IconContainer::AsyncRecord& m_record;
    AsyncIcon(GraphicsLibrary::Impl* impl, const PixmapKeyList& resourceCodes);

    bool  exists() const override { return m_record.exists(); }
    bool  isLoaded() const override { return m_record.isLoaded(); }
    bool  preload() const override { return m_record.preload(); }
    QIcon get() const override { return m_record.get(); }
};

class GraphicsLibrary::AsyncMovie : public IAsyncMovie {
public:
    GraphicsLibrary::Impl* m_impl;
    std::string            m_resourceName;
    bool                   m_exists = false;
    AsyncMovie(GraphicsLibrary::Impl* impl, const std::string& resourceName);

    bool    exists() const override { return m_exists; }
    bool    isLoaded() const override { return true; }
    bool    preload() const override { return true; }
    QMovie* create(QObject* parent) const override;
};

struct GraphicsLibrary::Impl {
    const Core::IResourceLibrary* m_resourceLibrary;

    CacheContainer<std::string, SpritePtr> m_spriteCache;
    CacheContainer<PixmapKey, QPixmap>     m_pixmapCache;
    CacheContainer<PixmapKeyList, QIcon>   m_iconCache;

    bool      createSprite(const std::string& resourceName, SpritePtr& result);
    bool      checkSprite(const std::string& resourceName);
    SpritePtr getCachedSprite(const std::string& resourceName);

    bool    createPixmap(const PixmapKey& resourceCode, QPixmap& result);
    QPixmap getCachedPixmap(const PixmapKey& resourceCode);

    bool createIcon(const PixmapKeyList& resourceCodes, QIcon& result);

    QMovie* createVideo(const std::string& resourceName, QObject* parent);

    bool checkVideo(const std::string& resourceName)
    {
        return m_resourceLibrary->fileExists(ResourceType::Video, resourceName);
    }

    Impl(const Core::IResourceLibrary* resourceLibrary)
        : m_resourceLibrary(resourceLibrary)
    {
        m_spriteCache.m_existCheck = [this](const std::string& resourceName) { return checkSprite(resourceName); };
        m_pixmapCache.m_existCheck = [this](const PixmapKey& resourceCode) { return checkSprite(resourceCode.resourceName); };
        m_iconCache.m_existCheck   = [this](const PixmapKeyList& resourceCodes) {
            for (const auto& code : resourceCodes)
                if (!checkSprite(code.resourceName))
                    return false;
            return true;
        };
        m_spriteCache.m_factory = [this](const std::string& resourceName, SpritePtr& result) { return createSprite(resourceName, result); };
        m_pixmapCache.m_factory = [this](const PixmapKey& resourceCode, QPixmap& result) { return createPixmap(resourceCode, result); };
        m_iconCache.m_factory   = [this](const PixmapKeyList& resourceCodes, QIcon& result) { return createIcon(resourceCodes, result); };
    }
};

GraphicsLibrary::GraphicsLibrary(const Core::IResourceLibrary* resourceLibrary)
    : m_impl(std::make_unique<Impl>(resourceLibrary))
{
}

GraphicsLibrary::~GraphicsLibrary()
{
}

IAsyncSpritePtr GraphicsLibrary::getObjectAnimation(const std::string& resourceName) const
{
    return std::make_shared<AsyncSprite>(m_impl.get(), resourceName);
}

IAsyncPixmapPtr GraphicsLibrary::getPixmapByKey(const PixmapKey& resourceCode) const
{
    return std::make_shared<AsyncPixmap>(m_impl.get(), resourceCode);
}

IAsyncMoviePtr GraphicsLibrary::getVideo(const std::string& resourceName) const
{
    return std::make_shared<AsyncMovie>(m_impl.get(), resourceName);
}

IAsyncIconPtr GraphicsLibrary::getIcon(const PixmapKeyList& resourceCodes) const
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

bool GraphicsLibrary::Impl::createSprite(const std::string& resourceName, SpritePtr& result)
{
    if (!m_resourceLibrary->fileExists(ResourceType::Sprite, resourceName)) {
        Mernel::Logger(Mernel::Logger::Notice) << "createSprite() failed, id does not exist: " << resourceName;
        return false;
    }

    const auto path       = m_resourceLibrary->get(ResourceType::Sprite, resourceName);
    auto       spriteImpl = std::make_shared<Sprite>();
    try {
        spriteImpl->load(path);
    }
    catch (std::exception& ex) {
        Mernel::Logger(Mernel::Logger::Err) << "Failed to load sprite:" << path << ", " << ex.what();
        return false;
    }

    if (spriteImpl->getGroupsCount() > 0) {
        result = spriteImpl;
        return true;
    }

    return false;
}

bool GraphicsLibrary::Impl::checkSprite(const std::string& resourceName)
{
    return m_resourceLibrary->fileExists(ResourceType::Sprite, resourceName);
}

SpritePtr GraphicsLibrary::Impl::getCachedSprite(const std::string& resourceName)
{
    return m_spriteCache.makeAsyncRecord(resourceName).get();
}

bool GraphicsLibrary::Impl::createPixmap(const PixmapKey& resourceCode, QPixmap& result)
{
    SpritePtr sprite = getCachedSprite(resourceCode.resourceName);
    if (!sprite)
        return false;

    if (!sprite->getGroupsIds().contains(resourceCode.group))
        return false;

    auto seq = sprite->getFramesForGroup(resourceCode.group);
    if (resourceCode.frame >= seq->m_frames.size() || resourceCode.frame < 0) // @todo: maybe allow Python-like [-1] = last elem etc.
        return false;

    result = seq->m_frames[resourceCode.frame].m_frame;
    if (seq->m_boundarySize != result.size()) {
        QImage padded{ seq->m_boundarySize, QImage::Format_RGBA8888 };
        padded.fill(Qt::transparent);
        QPainter p{ &padded };
        p.drawPixmap(seq->m_frames[resourceCode.frame].m_paddingLeftTop, result);
        result = QPixmap::fromImage(padded);
    }
    return true;
}

QPixmap GraphicsLibrary::Impl::getCachedPixmap(const PixmapKey& resourceCode)
{
    return m_pixmapCache.makeAsyncRecord(resourceCode).get();
}

QMovie* GraphicsLibrary::Impl::createVideo(const std::string& resourceName, QObject* parent)
{
    const auto path  = m_resourceLibrary->get(ResourceType::Video, resourceName);
    QString    qpath = stdPath2QString(path);
    QMovie*    movie = new QMovie(qpath, QByteArray(), parent);
    return movie;
}

bool GraphicsLibrary::Impl::createIcon(const PixmapKeyList& resourceCodes, QIcon& result)
{
    // normal, pressed, disabled, hovered.
    // normal    = QIcon::Off +  QIcon::Normal
    // pressed   = QIcon::On  +  QIcon::Normal
    // disabled  = QIcon::Off +  QIcon::Disabled
    // hovered   = QIcon::Off +  QIcon::Selected
    QList<QPixmap> pixmaps;
    for (auto& resourceId : resourceCodes) {
        pixmaps << (resourceId.resourceName.empty() ? QPixmap() : getCachedPixmap(resourceId));
    }
    result = QIcon(pixmaps[0]);
    if (pixmaps.size() > 1 && !pixmaps[1].isNull()) {
        result.addPixmap(pixmaps[1], QIcon::Normal, QIcon::On);
    }
    if (pixmaps.size() > 2 && !pixmaps[2].isNull()) {
        result.addPixmap(pixmaps[2], QIcon::Disabled, QIcon::Off);
    }
    if (pixmaps.size() > 3 && !pixmaps[3].isNull()) {
        result.addPixmap(pixmaps[3], QIcon::Selected, QIcon::Off);
    }
    return true;
}

GraphicsLibrary::AsyncSprite::AsyncSprite(Impl* impl, const std::string& resourceName)
    : m_record(impl->m_spriteCache.makeAsyncRecord(resourceName))
{
}

GraphicsLibrary::AsyncPixmap::AsyncPixmap(Impl* impl, const PixmapKey& resourceCode)
    : m_record(impl->m_pixmapCache.makeAsyncRecord(resourceCode))
{
}

GraphicsLibrary::AsyncIcon::AsyncIcon(Impl* impl, const PixmapKeyList& resourceCodes)
    : m_record(impl->m_iconCache.makeAsyncRecord(resourceCodes))
{
}

GraphicsLibrary::AsyncMovie::AsyncMovie(Impl* impl, const std::string& resourceName)
    : m_impl(impl)
    , m_resourceName(resourceName)
    , m_exists(m_impl->checkVideo(m_resourceName))
{}

QMovie* GraphicsLibrary::AsyncMovie::create(QObject* parent) const
{
    return m_impl->createVideo(m_resourceName, parent);
}

}
