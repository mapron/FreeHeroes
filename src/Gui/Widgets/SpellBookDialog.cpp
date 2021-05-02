/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpellBookDialog.hpp"

#include "HoverHelper.hpp"
#include "CustomFrames.hpp"
#include "ResizePixmap.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"

// Core:
#include "LibrarySpell.hpp"

#include <QPainter>
#include <QPushButton>
#include <QLabel>
#include <QBoxLayout>
#include <QMouseEvent>

#include <array>

namespace FreeHeroes::Gui {

class TransparentButton : public QPushButton {
public:
    TransparentButton(QWidget* parent)
        : QPushButton(parent)
    {}

    void paintEvent(QPaintEvent* e) override {}
};

class TransparentLabel : public QLabel {
public:
    std::function<void()> onClick;
    TransparentLabel(QWidget* parent)
        : QLabel(parent)
    {
        setFrameStyle(QFrame::NoFrame);
        setAttribute(Qt::WA_Hover);
    }
    void mousePressEvent(QMouseEvent* ev) override
    {
        QLabel::mousePressEvent(ev);
        if (ev->button() == Qt::LeftButton && onClick)
            onClick();
    }
};

class SpellBg : public QFrame {
public:
    QPixmap                background;
    std::array<QPixmap, 5> spelltabs;
    std::array<QPixmap, 5> spellTitles;
    int                    currentTab    = 0;
    const int              topPadding    = 4;
    const int              leftPadding   = 4;
    const int              rightPadding  = 4;
    const int              bottomPadding = 25;
    const int              width         = 800;
    const int              height        = 600;
    const int              remainWidth   = width - leftPadding - rightPadding;
    const int              remainHeight  = height - topPadding - bottomPadding;
    const int              spellTop      = topPadding + 80;
    const int              spellLeft     = leftPadding + remainWidth - 88;
    const int              pagesTop      = topPadding + 58;
    const int              leftPageLeft  = leftPadding + 105;
    const int              rightPageLeft = 418;
    const int              cellHeight    = 96;
    const int              cellWidth     = 93;

    SpellBg(QWidget* parent)
        : QFrame(parent)
    {
        setFixedSize(remainWidth, remainHeight);
        setContentsMargins(0, 0, 0, 0);
        setFrameStyle(QFrame::Panel);
        setProperty("borderStyle", "common2");
        setLineWidth(0);
    };
    void paintEvent(QPaintEvent* ev)
    {
        QPainter p(this);

        p.drawPixmap(QRect(0, 0, remainWidth, remainHeight), background, QRect(leftPadding, topPadding, remainWidth, remainHeight));

        QPixmap& curSpellTab = spelltabs[currentTab];
        p.drawPixmap(spellLeft, spellTop, curSpellTab);

        if (currentTab < 4) {
            QPixmap& curSpellTitle = spellTitles[currentTab];
            p.drawPixmap(leftPageLeft, pagesTop - 30, curSpellTitle);
        }

        QFrame::paintEvent(ev);
    }
    void setTabIndex(int index)
    {
        currentTab = index;
        update();
    }
};

class SpellCell : public QWidget {
    TransparentLabel*          m_spellIcon;
    TransparentLabel*          m_spellDescr;
    Core::LibrarySpellConstPtr m_spell      = nullptr;
    GuiSpellConstPtr           m_guiSpell   = nullptr;
    bool                       m_allowCast  = false;
    bool                       m_enoughMana = false;

public:
    SpellCell(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* layout     = new QVBoxLayout(this);
        QHBoxLayout* iconLayout = new QHBoxLayout();
        iconLayout->setSpacing(0);
        iconLayout->setMargin(0);
        m_spellIcon  = new TransparentLabel(this);
        m_spellDescr = new TransparentLabel(this);
        QFont f      = m_spellDescr->font();
        f.setPointSize(f.pointSize() - 1);
        m_spellDescr->setFont(f);
        layout->addLayout(iconLayout);
        iconLayout->addSpacing(9);
        iconLayout->addWidget(m_spellIcon);
        iconLayout->addSpacing(9);
        layout->addWidget(m_spellDescr);
        layout->setSpacing(1);
        layout->setMargin(0);
        m_spellIcon->setFixedSize(78, 65);
        m_spellDescr->setFixedWidth(96);
        m_spellDescr->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        this->setFixedHeight(90);
    }
    void setupHover(HoverHelper* helper)
    {
        helper->addAlias(m_spellDescr, m_spellIcon);
    }
    void clear()
    {
        m_spellIcon->setPixmap({});
        m_spellDescr->setText({});
        m_spellDescr->setProperty("hoverName", {});
        m_spell = nullptr;
    }
    void updateData(const SpellsModel*                       spellsModel,
                    const Core::AdventureHero::SpellDetails& details,
                    bool                                     allowCast,
                    bool                                     enoughMana)
    {
        const int manaCost     = details.manaCost;
        const int schoolLevel  = details.level;
        const int hintDamage   = details.hintDamage;
        m_spell                = details.spell;
        m_guiSpell             = spellsModel->find(m_spell);
        m_allowCast            = allowCast;
        m_enoughMana           = enoughMana;
        const QString title    = m_guiSpell->spellBookInfo(manaCost, false);
        const QString titleUpd = enoughMana ? title : QString("<font color=\"#EBB923\">%1</font>").arg(title);

        const QString hoverName = m_guiSpell->spellBookInfo(manaCost, true);

        const QString descr = m_guiSpell->getDescription(schoolLevel, hintDamage);

        QPixmap icon      = m_guiSpell->getIconTrans();
        QPixmap iconLarge = m_guiSpell->getIconScroll();
        m_spellIcon->setPixmap(icon);

        m_spellDescr->setText(titleUpd);

        m_spellDescr->setProperty("hoverName", hoverName);
        m_spellDescr->setProperty("popupDescr", descr);
        m_spellDescr->setProperty("popupPixmap", iconLarge);
        m_spellDescr->setProperty("popupBottom", m_guiSpell->getName());
        m_spellDescr->setProperty("popupAllowModal", false);
    }

    void setPopupOffset(QPoint offset, bool isRight)
    {
        m_spellDescr->setProperty("popupOffset", offset);
        m_spellDescr->setProperty("popupOffsetAnchorHor", isRight ? "right" : "left");
    }

    void setOnClick(std::function<void(Core::LibrarySpellConstPtr)> cb)
    {
        m_spellDescr->onClick = [cb, this]() {
            if (m_enoughMana && m_allowCast)
                cb(m_spell);
        };
        m_spellIcon->onClick = [cb, this]() {
            if (m_enoughMana && m_allowCast)
                cb(m_spell);
        };
    }
};

struct SpellBookDialog::Impl {
    const SpellsModel* spellsModel;
    HoverHelper*       hoverHelper   = nullptr;
    SpellBg*           spellBg       = nullptr;
    TransparentLabel*  manaLabel     = nullptr;
    TransparentLabel*  nextPage      = nullptr;
    TransparentLabel*  prevPage      = nullptr;
    bool               showAdventure = false;
    int                page          = 0;

    QList<SpellCell*> pageCells;

    Core::AdventureHero::SpellList spellList;
    int                            mana = 0;

    Core::LibrarySpellConstPtr selectedSpell      = nullptr;
    bool                       allowCastAdventure = false;
    bool                       allowCastBattle    = false;

    Impl(const SpellsModel* spellsModel)
        : spellsModel(spellsModel)
    {}
};

SpellBookDialog::SpellBookDialog(const Core::AdventureHero::SpellList& spellList,
                                 const SpellsModel*                    spellsModel,
                                 const UiCommonModel*                  ui,
                                 int                                   mana,
                                 bool                                  allowAdventureCast,
                                 bool                                  allowBattleCast,
                                 QWidget*                              parent)
    : QDialog(parent)
    , m_impl(std::make_unique<Impl>(spellsModel))
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    this->setFixedSize(800 + 2, 600 - 2);

    m_impl->hoverHelper = new HoverHelper(this);
    m_impl->spellBg     = new SpellBg(this);
    m_impl->spellBg->move(5, 5);

    QFrame* mainFrame = new QFrame(this);
    mainFrame->setFixedSize(800 + 2, 600 - 2);
    mainFrame->setProperty("borderStyle", "common2");
    mainFrame->move(0, 0);
    mainFrame->setLineWidth(0);
    mainFrame->setFrameStyle(QFrame::Panel);

    const QSize bottomButtonSize{ 35, 45 };
    const QSize leftVisibleSize{ 85, 45 };
    const QSize leftLogicSize{ 85, 60 };
    const int   bookBottom             = 463;
    const int   bookBottomOffsetBattle = 312;
    const int   bookBottomOffsetGlobal = bookBottomOffsetBattle + 134;
    const int   bookBottomOffsetMana   = bookBottomOffsetBattle + 230;
    const int   bookBottomOffsetClose  = bookBottomOffsetBattle + 320;

    TransparentButton* btnClose = new TransparentButton(this);
    btnClose->setFixedSize(bottomButtonSize);
    btnClose->move(bookBottomOffsetClose, bookBottom);
    btnClose->setProperty("hoverName", tr("Close"));
    connect(btnClose, &QPushButton::clicked, this, &SpellBookDialog::close);
    m_impl->hoverHelper->addWidgets({ btnClose });

    m_impl->manaLabel = new TransparentLabel(this);
    m_impl->manaLabel->setProperty("hoverName", ui->skillInfo[Core::HeroPrimaryParamType::Mana].name);
    m_impl->manaLabel->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    m_impl->manaLabel->setFixedSize(bottomButtonSize);
    m_impl->manaLabel->move(bookBottomOffsetMana, bookBottom);
    m_impl->hoverHelper->addWidgets({ m_impl->manaLabel });

    TransparentButton* btnBattle = new TransparentButton(this);
    btnBattle->setFixedSize(bottomButtonSize);
    btnBattle->move(bookBottomOffsetBattle, bookBottom);
    btnBattle->setProperty("hoverName", tr("Battle spells"));
    connect(btnBattle, &QPushButton::clicked, this, &SpellBookDialog::showBattle);
    m_impl->hoverHelper->addWidgets({ btnBattle });

    TransparentButton* btnGlobal = new TransparentButton(this);
    btnGlobal->setFixedSize(bottomButtonSize);
    btnGlobal->move(bookBottomOffsetGlobal, bookBottom);
    btnGlobal->setProperty("hoverName", tr("Global spells"));
    connect(btnGlobal, &QPushButton::clicked, this, &SpellBookDialog::showAdventure);
    m_impl->hoverHelper->addWidgets({ btnGlobal });

    QStringList tabTitle{ tr("Air spells"), tr("Earth spells"), tr("Fire spells"), tr("Water spells"), tr("All spells") };

    for (int i = 0; i < 5; i++) {
        TransparentButton* btnSpell = new TransparentButton(this);
        btnSpell->setFixedSize(leftVisibleSize);
        btnSpell->move(m_impl->spellBg->spellLeft, m_impl->spellBg->spellTop + leftLogicSize.height() * i);
        btnSpell->setProperty("hoverName", tabTitle[i]);
        connect(btnSpell, &QPushButton::clicked, this, [this, i] { changeCurrentTab(i); });
        m_impl->hoverHelper->addWidgets({ btnSpell });
    }

    for (int i = 0; i < 24; i++) {
        auto* cell = new SpellCell(this);
        cell->setupHover(m_impl->hoverHelper);
        m_impl->pageCells << cell;
        const bool sideRight  = i / 12 > 0;
        const int  row        = (i % 12) / 3;
        const int  col        = i % 3;
        const int  colOffset  = col * m_impl->spellBg->cellWidth;
        const int  rowOffset  = row * m_impl->spellBg->cellHeight;
        const int  sideOffset = sideRight ? m_impl->spellBg->rightPageLeft : m_impl->spellBg->leftPageLeft;
        QPoint     pos(sideOffset + colOffset, rowOffset + m_impl->spellBg->pagesTop);
        cell->move(pos);
        cell->setPopupOffset(QPoint(sideRight ? m_impl->spellBg->rightPageLeft - 42 : m_impl->spellBg->rightPageLeft, rowOffset + m_impl->spellBg->pagesTop - 50), sideRight);
        cell->setOnClick([this](Core::LibrarySpellConstPtr spell) { this->onSpellClick(spell); });
    }

    m_impl->nextPage = new TransparentLabel(this);
    m_impl->prevPage = new TransparentLabel(this);
    m_impl->nextPage->setFixedSize(29, 32);
    m_impl->prevPage->setFixedSize(29, 32);

    m_impl->prevPage->move(88, 40);
    m_impl->nextPage->move(684, 37);

    m_impl->nextPage->setProperty("hoverName", tr("Next page"));
    m_impl->prevPage->setProperty("hoverName", tr("Previous page"));

    m_impl->hoverHelper->addWidgets({ m_impl->nextPage, m_impl->prevPage });

    m_impl->prevPage->onClick = [this] { prevPage(); };
    m_impl->nextPage->onClick = [this] { nextPage(); };

    auto* hoverTooltip = new DarkFrameLabel(this);
    hoverTooltip->setProperty("borderStyle", "common2");
    hoverTooltip->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    hoverTooltip->setMargin(0);
    hoverTooltip->setFixedSize(800 + 2, 22);
    hoverTooltip->move(0, 600 - 25);
    m_impl->hoverHelper->setHoverLabel(hoverTooltip);

    m_impl->spellBg->background = ui->spellbook.background->get();
    m_impl->allowCastAdventure  = allowAdventureCast;
    m_impl->allowCastBattle     = allowBattleCast;
    if (allowAdventureCast)
        m_impl->showAdventure = true;

    for (int i = 0; i < 5; i++) {
        m_impl->spellBg->spelltabs[i] = ui->spellbook.spelltabs[i]->get();
        if (i < 4)
            m_impl->spellBg->spellTitles[i] = ui->spellbook.spellTitles[i]->get();
    }
    m_impl->manaLabel->setText(QString("%1").arg(mana));
    m_impl->spellList = spellList;
    m_impl->mana      = mana;
    m_impl->prevPage->setPixmap(ui->spellbook.prevPage->get());
    m_impl->nextPage->setPixmap(ui->spellbook.nextPage->get());
    fillCurrentPage();
}

Core::LibrarySpellConstPtr SpellBookDialog::getSelectedSpell() const
{
    return m_impl->selectedSpell;
}

void SpellBookDialog::showBattle()
{
    m_impl->page          = 0;
    m_impl->showAdventure = false;
    fillCurrentPage();
}

void SpellBookDialog::showAdventure()
{
    m_impl->page          = 0;
    m_impl->showAdventure = true;
    fillCurrentPage();
}

void SpellBookDialog::nextPage()
{
    m_impl->page++;
    fillCurrentPage();
}

void SpellBookDialog::prevPage()
{
    m_impl->page--;
    fillCurrentPage();
}

void SpellBookDialog::changeCurrentTab(int index)
{
    m_impl->page = 0;
    m_impl->spellBg->setTabIndex(index);
    fillCurrentPage();
}

void SpellBookDialog::fillCurrentPage()
{
    std::vector<Core::AdventureHero::SpellDetails> currentPageSpells;
    const int                                      currentSchoolIndex = m_impl->spellBg->currentTab;
    const bool                                     anyTab             = currentSchoolIndex == 4;
    const int                                      tabOffset          = anyTab ? 0 : 2;
    const size_t                                   maxSize            = 24 - tabOffset;

    const std::vector<Core::MagicSchool> pageSchools{
        Core::MagicSchool::Air,
        Core::MagicSchool::Earth,
        Core::MagicSchool::Fire,
        Core::MagicSchool::Water,
        Core::MagicSchool::Any
    };
    const Core::MagicSchool currentSchool = pageSchools[currentSchoolIndex];

    for (auto& spellRec : m_impl->spellList) {
        const bool isAdventure = spellRec.spell->type == Core::LibrarySpell::Type::Adventure;
        if (isAdventure != m_impl->showAdventure)
            continue;
        if (currentSchool != Core::MagicSchool::Any && spellRec.spell->school != Core::MagicSchool::Any) {
            if (currentSchool != spellRec.spell->school)
                continue;
        }
        currentPageSpells.push_back(spellRec);
    }
    m_impl->spellBg->update();
    if (anyTab) {
        m_impl->pageCells[0]->clear();
        m_impl->pageCells[1]->clear();
    }

    const size_t totalPageCount = currentPageSpells.size() / maxSize;
    const size_t spellsOffset   = m_impl->page * maxSize;
    m_impl->prevPage->setVisible(m_impl->page > 0);
    m_impl->nextPage->setVisible(m_impl->page < int(totalPageCount));
    for (int i = 0; i < tabOffset; i++)
        m_impl->pageCells[i]->clear();

    for (size_t i = 0; i < maxSize; i++) {
        if ((i + spellsOffset) < currentPageSpells.size()) {
            const auto& spellInfo   = currentPageSpells[i + spellsOffset];
            const bool  isAdventure = spellInfo.spell->type == Core::LibrarySpell::Type::Adventure;
            const bool  allowCast   = ((isAdventure && m_impl->allowCastAdventure)
                                    || (!isAdventure && m_impl->allowCastBattle));

            const bool enoughMana = spellInfo.manaCost <= m_impl->mana;
            m_impl->pageCells[i + tabOffset]->updateData(m_impl->spellsModel,
                                                         spellInfo,
                                                         allowCast,
                                                         enoughMana);
        } else
            m_impl->pageCells[i + tabOffset]->clear();
    }
}

void SpellBookDialog::onSpellClick(Core::LibrarySpellConstPtr spell)
{
    if (!spell)
        return;

    m_impl->selectedSpell = spell;
    close();
}

SpellBookDialog::~SpellBookDialog() = default;

}
