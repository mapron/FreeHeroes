/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ArtifactPuppetWidget.hpp"

// Gui
#include "CustomFrames.hpp"
#include "ResizePixmap.hpp"
#include "HoverHelper.hpp"
#include "DependencyInjector.hpp"
#include "AdventureWrappers.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"

// Core
#include "IAdventureControl.hpp"
#include "AdventureHero.hpp"
#include "LibraryArtifact.hpp"

#include <QBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QApplication>
#include <QModelIndex>
#include <QMouseEvent>
#include <QDebug>

namespace FreeHeroes::Gui {
using namespace Core;

class PuppetBack : public QWidget {
    QPixmap m_back;

public:
    PuppetBack(QWidget* parent)
        : QWidget(parent)
    {
        m_back = QPixmap::fromImage(QImage(":/Application/puppet_back.png"));
    }

    void paintEvent(QPaintEvent* e) override
    {
        QPainter p(this);
        p.drawPixmap(QPoint(15, 12), m_back);
        QWidget::paintEvent(e);
    }
};

class ArtifactPuppetWidget::ArtifactButton : public DarkFrameLabel {
public:
    bool m_isDraggable          = true;
    bool m_isBag                = false;
    bool m_acceptDrops          = false;
    bool m_isLock               = false;
    bool m_allowModal           = false;
    bool m_assembleAvailable    = false;
    bool m_disassembleAvailable = false;

    const ArtifactsModel*                    m_artifactsModel = nullptr;
    std::function<GuiArtifactConstPtr(void)> m_customArtGetter;
    std::function<void(void)>                m_leftClick;

    ArtifactButton(Core::ArtifactSlotType wearingSlot,
                   ArtifactPuppetWidget*  parent,
                   QWidget*               drawParent)
        : DarkFrameLabel(drawParent)
        , m_parent(parent)
        , m_wearingSlot(wearingSlot)
    {
        setFixedSize({ 48, 48 });
    }
    void setHero(Core::AdventureHeroConstPtr hero)
    {
        m_hero = hero;
        updatePixmap();
    }

    void updatePixmap()
    {
        auto art = getCurrentArtifact();
        m_isLock = false;
        if (!art) {
            m_isLock = m_hero->estimated.slotsInfo.extraWearing.wearingSlots.contains(m_wearingSlot);
        }
        if (!art) {
            if (m_isLock) {
                auto img = m_artifactsModel->getLockIcon();
                img      = resizePixmap(img, { 44, 44 }, true);
                this->setPixmap(img);
                this->setProperty("hoverName", QObject::tr("Blocked"));
            } else {
                this->setPixmap({});
                this->setProperty("hoverName", QObject::tr("Empty"));
            }
            this->setProperty("popupDescr", "");
            return;
        }
        auto img = art->getIconStash();
        img      = resizePixmap(img, { 44, 44 }, true);
        this->setPixmap(img);
        int                     setPartsCount = -1;
        LibraryArtifactConstPtr missingPart   = nullptr;
        m_assembleAvailable                   = false;
        m_disassembleAvailable                = false;
        if (!m_isBag && art->getSource()->partOfSet) {
            setPartsCount = (int) m_hero->getWearingCountFromSet(art->getSource()->partOfSet);
            auto missing  = m_hero->getMissingPartsFromSet(art->getSource()->partOfSet);
            if (missing.size() == 1)
                missingPart = missing[0];
            m_assembleAvailable = missing.empty();
        }

        QString descr = art->getDescr();
        if (setPartsCount >= 0) {
            descr += "<br><br>" + m_artifactsModel->find(art->getSource()->partOfSet)->localizedPartSetDescr(setPartsCount, missingPart);
        }
        if (art->getSource()->parts.size()) {
            descr += "<br><br>" + art->localizedDisassemblySetDescr();
            m_disassembleAvailable = true;
        }
        this->setProperty("popupDescr", descr);

        this->setProperty("hoverName", art->getName());
        this->setProperty("popupAllowModal", m_allowModal);
    }

    void checkAcceptDrops(Core::ArtifactSlotType compatSlot)
    {
        m_acceptDrops = !m_isLock && compatSlot != Core::ArtifactSlotType::Invalid && compatSlot == ArtifactWearingSet::getCompatibleSlotType(m_wearingSlot);
        update();
    }

    void paintEvent(QPaintEvent* e) override
    {
        if (this->property("isHovered").toBool()) {
            QPainter        p(this);
            QRect           r         = rect().adjusted(0, 0, -1, -1);
            const qreal     boxWidth  = r.width();
            const qreal     boxHeight = r.height();
            QRadialGradient grad(QPointF{ boxWidth / 2, boxHeight / 2 }, boxWidth * 0.8);
            grad.setColorAt(0, QColor(255, 255, 255, 70));
            grad.setColorAt(1, QColor(255, 255, 255, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(QBrush(grad));
            p.drawRect(r);
        }

        DarkFrameLabel::paintEvent(e);
        if (m_acceptDrops) {
            QPainter p(this);
            p.setPen(QColor(255, 255, 255, 150));
            QRect r = rect().adjusted(0, 0, -1, -1);
            p.drawRect(r);
        }
    }

    void wheelEvent(QWheelEvent* event) override
    {
        QPoint delta = event->angleDelta();
        //qWarning() << delta;
        // try vertical scroll at first; if mouse have horizontal scroll, try it as second.
        auto guessScrollSize = [](int angle) {
            const int approx = angle / 120;
            if (approx != 0)
                return -approx;
            if (angle < 0)
                return 1;
            if (angle > 0)
                return 1;
            return 0;
        };
        if (delta.y() != 0) {
            m_parent->scrollBag(guessScrollSize(delta.y()));
        } else if (delta.x() != 0) {
            m_parent->scrollBag(guessScrollSize(delta.x()));
        }
    }

    void mousePressEvent(QMouseEvent* e) override;

private:
    GuiArtifactConstPtr getCurrentArtifact() const
    {
        if (m_customArtGetter)
            return m_customArtGetter();

        auto art = m_hero->getArtifact(m_wearingSlot);
        return art ? m_artifactsModel->find(art) : nullptr;
    }

private:
    ArtifactPuppetWidget*       m_parent;
    Core::ArtifactSlotType      m_wearingSlot;
    Core::AdventureHeroConstPtr m_hero;
};

class ArtifactPuppetWidget::BagListModel : public QObject {
    AdventureHeroConstPtr                m_hero  = nullptr;
    int                                  m_shift = 0;
    std::vector<LibraryArtifactConstPtr> m_allBag;

public:
    static const int windowSize = 5;

    BagListModel(QObject* parent)
        : QObject(parent)
    {
    }

    void setHero(AdventureHeroConstPtr hero)
    {
        m_hero = hero;
        fillBag();
    }

    void fillBag()
    {
        m_allBag.clear();
        for (auto p : m_hero->artifactsBag) {
            LibraryArtifactConstPtr art = p.first;
            for (int i = 0; i < p.second; ++i) {
                m_allBag.push_back(art);
            }
        }
        std::sort(m_allBag.begin(), m_allBag.end(), [](auto* l, auto* r) {
            return l->sortOrdering() < r->sortOrdering();
        });
    }

    void scroll(int delta)
    {
        m_shift += delta;
        if (m_allBag.size() <= windowSize) {
            m_shift = 0;
            return;
        }

        while (m_shift < 0)
            m_shift += m_allBag.size();
        while (m_shift > (int) m_allBag.size())
            m_shift -= m_allBag.size();
    }
    void scrollLeft() { scroll(-1); }
    void scrollRight() { scroll(1); }

    int getBagCount() const
    {
        return m_allBag.size();
    }
    int rowCount() const { return std::max(getBagCount(), windowSize); }

    LibraryArtifactConstPtr getWindowArtifact(int index) const
    {
        const int totalSize = getBagCount();
        if (totalSize < windowSize && index >= totalSize)
            return nullptr;

        const int shift      = totalSize < windowSize ? 0 : m_shift;
        const int exactIndex = (index + shift) % totalSize;

        return m_allBag[exactIndex];
    }

    void refresh()
    {
        fillBag();
    }
};

struct ArtifactPuppetWidget::Impl {
    ArtifactPuppetWidget*  m_parent;
    AdventureHeroConstPtr  m_hero                 = nullptr;
    IAdventureHeroControl* m_adventureHeroControl = nullptr;

    const LibraryModelsProvider* const m_modelProvider  = nullptr;
    const ArtifactsModel* const        m_artifactsModel = nullptr;
    BagListModel*                      m_listModel      = nullptr;

    FlatButton*  m_scrollLeft   = nullptr;
    FlatButton*  m_scrollRight  = nullptr;
    QHBoxLayout* m_bottomLayout = nullptr;

    QWidget* m_puppetWidget = nullptr;

    QList<ArtifactButton*> m_buttons;
    QList<ArtifactButton*> m_buttonsBag;

    LibraryArtifactConstPtr m_dragArtifact   = nullptr;
    Core::ArtifactSlotType  m_dragSourceSlot = Core::ArtifactSlotType::Invalid;
    bool                    m_dragSourceBag  = false;

    HoverHelper* m_hoverHelper = nullptr;

    Impl(const LibraryModelsProvider* modelProvider, ArtifactPuppetWidget* parent)
        : m_parent(parent)
        , m_modelProvider(modelProvider)
        , m_artifactsModel(modelProvider->artifacts())
    {}

    void startDrag(LibraryArtifactConstPtr art, Core::ArtifactSlotType slot, bool fromBag)
    {
        m_dragArtifact   = art;
        m_dragSourceSlot = slot;
        m_dragSourceBag  = fromBag;
        auto img         = m_artifactsModel->find(art)->getIconStash();
        img              = resizePixmap(img, { 44, 44 }, true);
        QCursor cur(img, 22, 22);
        QApplication::setOverrideCursor(cur);
        for (auto* btn : m_buttons) {
            btn->checkAcceptDrops(art->slot);
        }
    }

    void stopDrag()
    {
        if (m_dragArtifact) {
            m_dragArtifact   = nullptr;
            m_dragSourceSlot = Core::ArtifactSlotType::Invalid;
            QApplication::restoreOverrideCursor();
            for (auto* btn : m_buttons) {
                btn->checkAcceptDrops(Core::ArtifactSlotType::Invalid);
            }
            for (auto* btn : m_buttonsBag) {
                btn->updatePixmap();
            }
        }
    }
};

ArtifactPuppetWidget::ArtifactPuppetWidget(const LibraryModelsProvider* modelProvider, QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<Impl>(modelProvider, this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_impl->m_puppetWidget = new PuppetBack(this);
    m_impl->m_hoverHelper  = new HoverHelper(modelProvider, this);
    //puppetWidget->setFixedHeight(150);
    layout->addWidget(m_impl->m_puppetWidget, 1);

    m_impl->m_bottomLayout = new QHBoxLayout();
    layout->addLayout(m_impl->m_bottomLayout);
    m_impl->m_bottomLayout->setSpacing(0);
    m_impl->m_bottomLayout->setContentsMargins(0, 0, 0, 0);

    m_impl->m_scrollLeft  = new FlatButton(this);
    m_impl->m_scrollRight = new FlatButton(this);

    m_impl->m_listModel = new BagListModel(this);

    m_impl->m_bottomLayout->addWidget(m_impl->m_scrollLeft);
    for (int i = 0; i < BagListModel::windowSize; i++) {
        Core::ArtifactSlotType compatSlot = Core::ArtifactSlotType::Invalid;
        auto*                  button     = new ArtifactButton(compatSlot,
                                          this,
                                          this);
        m_impl->m_bottomLayout->addWidget(button);
        m_impl->m_buttonsBag << button;
        m_impl->m_hoverHelper->addWidgets({ button });
        button->m_customArtGetter = [this, i] {
            return m_impl->m_artifactsModel->find(m_impl->m_listModel->getWindowArtifact(i));
        };
        button->m_isBag = true;

        button->setProperty("popupOffset", QPoint(50 + i * 48, 360));
        button->setProperty("popupOffsetAnchorHor", "center");
        button->setProperty("popupOffsetAnchorVert", "bottom");
    }
    m_impl->m_bottomLayout->addWidget(m_impl->m_scrollRight);

    m_impl->m_scrollLeft->setFixedSize({ 22, 46 });
    m_impl->m_scrollRight->setFixedSize({ 22, 46 });

    auto updateBag = [this] {for (auto * btn : m_impl->m_buttonsBag)  btn->updatePixmap(); };

    // clang-format off
    connect(m_impl->m_scrollLeft , &QPushButton::clicked, m_impl->m_listModel, &BagListModel::scrollLeft);
    connect(m_impl->m_scrollRight, &QPushButton::clicked, m_impl->m_listModel, &BagListModel::scrollRight);
    connect(m_impl->m_scrollLeft , &QPushButton::clicked, this, updateBag);
    connect(m_impl->m_scrollRight, &QPushButton::clicked, this, updateBag);
    // clang-format on

    auto addButton = [this](int x, int y, Core::ArtifactSlotType wearingSlot) -> ArtifactButton* {
        auto* button = new ArtifactButton(wearingSlot,
                                          this,
                                          m_impl->m_puppetWidget);
        button->move(x + 10, y + 12);
        m_impl->m_buttons << button;
        m_impl->m_hoverHelper->addWidgets({ button });
        int w = 0;
        int h = 48;
        if (x > 100) {
            w = 24;
            button->setProperty("popupOffsetAnchorHor", "center");
        }
        if (x > 170) {
            w = 48;
            button->setProperty("popupOffsetAnchorHor", "right");
        }
        if (y > 200) {
            h = 0;
            button->setProperty("popupOffsetAnchorVert", "bottom");
        }
        QPoint offset(x + w + 10, y + h + 12);
        button->setProperty("popupOffset", offset);
        return button;
    };
    const std::vector<int> xs{ 0, 16, 33, 49, 124, 180, 185, 228 };
    const std::vector<int> ys{ 0, 39, 48, 51, 96, 102, 113, 154, 164, 215, 266, 282 };

    addButton(xs[0], ys[1], Core::ArtifactSlotType::Sword);
    addButton(xs[3], ys[1], Core::ArtifactSlotType::Ring);

    addButton(xs[4], ys[0], Core::ArtifactSlotType::Helm);
    addButton(xs[4], ys[2], Core::ArtifactSlotType::Neck);
    addButton(xs[4], ys[5], Core::ArtifactSlotType::Torso);

    addButton(xs[5], ys[0], Core::ArtifactSlotType::BmShoot)->m_isDraggable = false;
    addButton(xs[7], ys[0], Core::ArtifactSlotType::BmAmmo)->m_isDraggable  = false;
    addButton(xs[7], ys[2], Core::ArtifactSlotType::BmTent)->m_isDraggable  = false;

    auto* cata              = addButton(xs[7], ys[4], Core::ArtifactSlotType::Invalid);
    cata->m_customArtGetter = [this] {
        auto art = m_impl->m_artifactsModel->findCatapult();
        return art;
    };
    cata->m_allowModal  = true;
    cata->m_isDraggable = false;

    addButton(xs[0], ys[6], Core::ArtifactSlotType::Misc);
    addButton(xs[1], ys[8], Core::ArtifactSlotType::Misc1);
    addButton(xs[2], ys[9], Core::ArtifactSlotType::Misc2);
    addButton(xs[0], ys[10], Core::ArtifactSlotType::Misc3);
    addButton(xs[3], ys[10], Core::ArtifactSlotType::Misc4);

    addButton(xs[5], ys[7], Core::ArtifactSlotType::Shield);
    addButton(xs[7], ys[7], Core::ArtifactSlotType::Ring1);
    addButton(xs[6], ys[9], Core::ArtifactSlotType::Cape);
    addButton(xs[4], ys[10], Core::ArtifactSlotType::Boots);

    auto* spellButton              = addButton(xs[7], ys[11], Core::ArtifactSlotType::Invalid);
    spellButton->m_customArtGetter = [this]() -> GuiArtifactConstPtr {
        return m_impl->m_hero->hasSpellBook ? m_impl->m_artifactsModel->findSpellbook() : nullptr;
    };
    spellButton->m_isDraggable = false;
    spellButton->m_leftClick   = [this] {
        if (m_impl->m_hero->hasSpellBook)
            emit openSpellBook();
    };

    for (auto& btn : m_impl->m_buttons) {
        btn->m_artifactsModel = m_impl->m_artifactsModel;
    }

    for (auto& btn : m_impl->m_buttonsBag) {
        btn->m_artifactsModel = m_impl->m_artifactsModel;
    }

    m_impl->m_scrollLeft->setIcon(modelProvider->ui()->buttons.scrollLeft->get());
    m_impl->m_scrollRight->setIcon(modelProvider->ui()->buttons.scrollRight->get());
}

void ArtifactPuppetWidget::refresh()
{
    m_impl->m_listModel->refresh();
    m_impl->m_scrollLeft->setEnabled(m_impl->m_listModel->getBagCount() > 5);
    m_impl->m_scrollRight->setEnabled(m_impl->m_listModel->getBagCount() > 5);

    for (auto& btn : m_impl->m_buttons)
        btn->updatePixmap();

    for (auto& btn : m_impl->m_buttonsBag)
        btn->updatePixmap();
}

ArtifactPuppetWidget::~ArtifactPuppetWidget()
{
    m_impl->stopDrag();
}

void ArtifactPuppetWidget::setHoverLabel(QLabel* hoverLabel)
{
    m_impl->m_hoverHelper->setHoverLabel(hoverLabel);
}

void ArtifactPuppetWidget::setSource(const GuiAdventureHero*      hero,
                                     Core::IAdventureHeroControl* adventureHeroControl)
{
    m_impl->m_hero = hero->getSource();
    m_impl->m_listModel->setHero(hero->getSource());

    m_impl->m_adventureHeroControl = adventureHeroControl;

    for (auto& btn : m_impl->m_buttons) {
        btn->setHero(hero->getSource());
    }

    for (auto& btn : m_impl->m_buttonsBag) {
        btn->setHero(hero->getSource());
    }
}

void ArtifactPuppetWidget::scrollBag(int delta)
{
    m_impl->m_listModel->scroll(delta);
    for (auto* btn : m_impl->m_buttonsBag)
        btn->updatePixmap();
}

void ArtifactPuppetWidget::ArtifactButton::mousePressEvent(QMouseEvent* e)
{
    DarkFrameLabel::mousePressEvent(e);
    if (e->button() != Qt::LeftButton) {
        return;
    }
    const bool noMods = e->modifiers() == Qt::NoModifier;
    //const bool shiftMod   = e->modifiers() == Qt::ShiftModifier;
    const bool controlMod = e->modifiers() == Qt::ControlModifier;
    const bool altMod     = e->modifiers() == Qt::AltModifier;

    if (m_leftClick && noMods) {
        m_leftClick();
        return;
    }
    if (!m_isDraggable)
        return;
    if (!m_parent->m_impl->m_dragArtifact) {
        auto art = getCurrentArtifact();
        if (!art)
            return;
        if (altMod && m_isBag) {
            Core::ArtifactSlotType targetSlot = art->getSource()->slot;
            for (auto slot : ArtifactWearingSet::getCompatibleWearingSlots(targetSlot)) {
                if (!m_parent->m_impl->m_hero->getArtifact(slot)) {
                    targetSlot = slot;
                    break;
                }
            }
            m_parent->m_impl->m_adventureHeroControl->heroArtifactPutOn({ targetSlot, art->getSource(), true });

            return;
        }
        if (altMod && !m_isBag) {
            m_parent->m_impl->m_adventureHeroControl->heroArtifactTakeOff({ m_wearingSlot });
            return;
        }
        if (controlMod && !m_isBag) {
            if (m_assembleAvailable)
                m_parent->m_impl->m_adventureHeroControl->heroArtifactAssembleSet({ m_wearingSlot });
            else if (m_disassembleAvailable)
                m_parent->m_impl->m_adventureHeroControl->heroArtifactDisassembleSet({ m_wearingSlot });
            return;
        }
        if (!m_isLock) {
            m_parent->m_impl->startDrag(art->getSource(), m_wearingSlot, m_isBag);
            this->setPixmap({});
        }
        return;
    }
    if (m_wearingSlot != Core::ArtifactSlotType::Invalid && ArtifactWearingSet::getCompatibleSlotType(m_wearingSlot) != m_parent->m_impl->m_dragArtifact->slot)
        return;

    // from bag to slot
    if (m_parent->m_impl->m_dragSourceBag && !m_isBag && !m_isLock) {
        if (m_parent->m_impl->m_adventureHeroControl->heroArtifactPutOn({ m_wearingSlot, m_parent->m_impl->m_dragArtifact }))
            m_parent->m_impl->stopDrag();
        return;
    }
    // bag-bag
    if (m_parent->m_impl->m_dragSourceBag && m_isBag) {
        m_parent->m_impl->stopDrag();
        updatePixmap();
        return;
    }
    // same slot drop
    if (!m_parent->m_impl->m_dragSourceBag && !m_isBag && m_wearingSlot == m_parent->m_impl->m_dragSourceSlot && !m_isLock) {
        m_parent->m_impl->stopDrag();
        updatePixmap();
        return;
    }
    // from slot to slot
    if (!m_parent->m_impl->m_dragSourceBag && !m_isBag && !m_isLock) {
        m_parent->m_impl->m_adventureHeroControl->heroArtifactSwap({ m_wearingSlot, m_parent->m_impl->m_dragSourceSlot });
        m_parent->m_impl->stopDrag();
        return;
    }
    // from slot to bag
    if (!m_parent->m_impl->m_dragSourceBag && m_isBag) {
        m_parent->m_impl->m_adventureHeroControl->heroArtifactTakeOff({ m_parent->m_impl->m_dragSourceSlot });
        m_parent->m_impl->stopDrag();
        return;
    }
}

}
