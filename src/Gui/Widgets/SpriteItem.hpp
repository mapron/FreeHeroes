/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "ISprites.hpp"

#include <QGraphicsItem>
#include <QVector>

namespace FreeHeroes::Gui {

class SpriteItem;

class GUIWIDGETS_EXPORT SporadicOrchestrator {
    QList<qlonglong> m_timestamps;
public:
    bool checkEventLimit(int interval, int maxCount);
};

struct GUIWIDGETS_EXPORT SporadicHandle {
    struct SporadicConfig {
        bool enabled = false;
        int delayMs = 0;
        int maxExtraDelayMs = 0;
        std::function<void()> action;
        std::function<bool()> check;
       // AnimGroupSettings anim;
    };

    SporadicConfig cfg;
    int currentDelay = 0;
    bool inSporadic = false;

    int frameMsTick = 0;

    SporadicHandle() = default;
    SporadicHandle(const SporadicConfig & cfg) : cfg(cfg) { reset(); }

    bool tick(int msecElapsed, bool ignoreCheck = false);
    void reset();
    void runNow();
};

class GUIWIDGETS_EXPORT SpriteItem : public QGraphicsItem
{
public:
    enum class DrawOriginH { Left, Center, Right };
    enum class DrawOriginV { Top, Center, Bottom };

    struct AnimGroupSettings {
        int group = -1;
        int durationMs = 0;
        int startFrame = 0;
        int repeatOnFrame = -1;
        int repeatOnFrameCount = -1;
        bool loopOver = false;
        bool reverse = false;
        AnimGroupSettings() = default;
        AnimGroupSettings(int group, int durationMs) : group(group), durationMs(durationMs) {}

        AnimGroupSettings & setReverse(bool val = true) { reverse = val; return *this; }
        AnimGroupSettings & setLoopOver(bool val = true) { loopOver = val; return *this; }
        AnimGroupSettings & setStartFrame(int frame) { startFrame = frame; return *this; }
        AnimGroupSettings & setRepeatFrame(int onFrame, int count) { repeatOnFrame = onFrame; repeatOnFrameCount = count; return *this; }

        bool isValid() const { return group > 0; }
    };



public:
    SpriteItem(QGraphicsItem* parent = nullptr);

    void setSprite(const SpritePtr & frames);
    const SpritePtr & getSprite() const;

    void setAnimGroup(const AnimGroupSettings & settings);

    void setAnimGroupSporadic(const AnimGroupSettings & settings);

    void setMirrorHor(bool state);
    void setMirrorVert(bool state);

    void triggerSporadic();
    void tick(int msecElapsed);

    void setDrawOriginH(DrawOriginH drawOrigin) { m_drawOriginH = drawOrigin; update();}
    void setDrawOriginV(DrawOriginV drawOrigin) { m_drawOriginV = drawOrigin; update();}

protected:
    void displayCurrentPixmap();

protected:

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    bool contains(const QPointF &point) const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    bool isObscuredBy(const QGraphicsItem *item) const override;
    QPainterPath opaqueArea() const override;

    enum { Type = UserType + 1 };
    int type() const override;




protected:
    void setAnimGroupInternal();

protected:
    SpritePtr m_sprite;
    SpriteSequencePtr m_currentSequence;

    DrawOriginH m_drawOriginH = DrawOriginH::Center;
    DrawOriginV m_drawOriginV = DrawOriginV::Center;
    QPixmap m_pixmap;
    QSizeF m_boundingSize;
    QPointF m_boundingOrigin;
    QPointF m_pixmapPadding;

    bool m_mirrorHor = false;
    bool m_mirrorVert = false;

    int m_frameMsTick = 0;
    int m_currentFrameIndex = 0;

    AnimGroupSettings m_animSettings;
    AnimGroupSettings m_sporadicSettings;

    bool m_inSporadic = false;
};

class GUIWIDGETS_EXPORT SpriteItemObj : public QObject, public SpriteItem
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
    Q_PROPERTY(qreal zValue READ zValue WRITE setZValue)
public:
    using SpriteItem::SpriteItem;

signals:
    void hoverIn();
    void hoverOut();
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
};

}
