/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "HeroLevelupDialog.hpp"

#include "DialogUtils.hpp"
#include "CustomFrames.hpp"

#include <array>

#include <QBoxLayout>
#include <QLabel>

namespace FreeHeroes::Gui {

struct HeroLevelupDialog::Impl {
    DarkFrameLabel* title = nullptr;
    DarkFrameLabelIcon* portrait = nullptr;
    DarkFrameLabel* levelAndClass = nullptr;
    DarkFrameLabelIcon* statPrimaryIcon = nullptr;
    DarkFrameLabel* statPrimaryTxt = nullptr;
    QLabel * choiceDescription = nullptr;
    FlatButton * leftChoice = nullptr;
    QLabel * leftChoiceDescr = nullptr;
    QLabel * orText = nullptr;
    FlatButton * rightChoice = nullptr;
    QLabel * rightChoiceDescr = nullptr;
    FlatButton * closeButton = nullptr;
    FlatLabelIcon * expLeft = nullptr;
    FlatLabelIcon * expRight = nullptr;

    int m_currentChoice = 0;

    std::array<FlatButton * , 2> choiceBtns;
    std::array<QLabel * , 2> choiceDescrs;

};

HeroLevelupDialog::HeroLevelupDialog(QWidget* parent)
    : QDialog(parent)
    , m_impl(std::make_unique<Impl>())

{
    setWindowModality(Qt::WindowModal);
    DialogUtils::commonDialogSetup(this);

    auto enlargeLabel= [](QLabel * lbl, int size, bool bold) {
       QFont f   = lbl->font();
       f.setPointSize(f.pointSize() + size);
       f.setBold(bold);
       lbl->setFont(f);
    };

    auto makeLabel = [this, enlargeLabel]() -> DarkFrameLabel* {
        auto * lbl = new DarkFrameLabel(this);
        lbl->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        lbl->setMargin(5);
        int h = lbl->fontMetrics().height();
        lbl->setMinimumHeight(h + 5);
        auto p = lbl->sizePolicy();
        p.setHorizontalPolicy(QSizePolicy::Expanding);
        lbl->setSizePolicy(p);
        enlargeLabel(lbl, 2, true);
        return lbl;
    };


    QVBoxLayout * mainLayout = DialogUtils::makeMainDialogFrame(this);
    mainLayout->setContentsMargins(10, 15, 10, 10);
    {
        mainLayout->addWidget((m_impl->title = makeLabel()));
        {

            QHBoxLayout * portraitWraps = new QHBoxLayout();
            mainLayout->addLayout(portraitWraps);
            portraitWraps->addSpacing(40);
            m_impl->expLeft = new FlatLabelIcon(this);

            portraitWraps->addWidget(m_impl->expLeft);
            portraitWraps->addSpacing(40);
            portraitWraps->addWidget((m_impl->portrait = new DarkFrameLabelIcon(this)));
            portraitWraps->addSpacing(40);
            m_impl->expRight = new FlatLabelIcon(this);
            portraitWraps->addWidget(m_impl->expRight);
            portraitWraps->addSpacing(40);

        }
        mainLayout->addWidget((m_impl->levelAndClass = makeLabel()));
        {
            QHBoxLayout * statPrimaryWraps = new QHBoxLayout();
            mainLayout->addLayout(statPrimaryWraps);
            statPrimaryWraps->addStretch();
            statPrimaryWraps->addWidget((m_impl->statPrimaryIcon = new DarkFrameLabelIcon(this)));
            statPrimaryWraps->addStretch();
        }
        mainLayout->addWidget((m_impl->statPrimaryTxt = makeLabel()));
    }
    {
        m_impl->choiceDescription = new QLabel(this);
        enlargeLabel(m_impl->choiceDescription, 2, false);

        m_impl->leftChoice       = new FlatButton(this);
        m_impl->leftChoiceDescr  = new QLabel(this);
        m_impl->orText           = new QLabel(tr("or") , this);
        m_impl->rightChoice      = new FlatButton(this);
        m_impl->rightChoiceDescr = new QLabel(this);
        m_impl->leftChoice->setFixedSize(42, 42);
        m_impl->rightChoice->setFixedSize(42, 42);
        m_impl->leftChoice->setBorderWidth(1, true);
        m_impl->rightChoice->setBorderWidth(1, true);
        enlargeLabel(m_impl->orText, 3, true);
        for (auto * lbl : {m_impl->choiceDescription, m_impl->leftChoiceDescr, m_impl->orText, m_impl->rightChoiceDescr}) {
            lbl->setAlignment(Qt::AlignCenter);
        }

        mainLayout->addWidget(m_impl->choiceDescription);
        {
            QHBoxLayout * choicesRow = new QHBoxLayout();
            mainLayout->addLayout(choicesRow);

            choicesRow->addStretch();
            {
                QVBoxLayout * choicesLeftWrap = new QVBoxLayout();
                choicesRow->addLayout(choicesLeftWrap);
                {
                    QHBoxLayout * choicesCentering = new QHBoxLayout();
                    choicesCentering->addStretch();
                    choicesCentering->addWidget(m_impl->leftChoice);
                    choicesCentering->addStretch();
                    choicesLeftWrap->addLayout(choicesCentering);
                }
                choicesLeftWrap->addWidget(m_impl->leftChoiceDescr);
            }
            {
                QVBoxLayout * choicesOrWrap = new QVBoxLayout();
                choicesRow->addLayout(choicesOrWrap);
                choicesOrWrap->addSpacing(10);
                choicesOrWrap->addWidget(m_impl->orText);
                choicesOrWrap->addStretch();
            }
            {
                QVBoxLayout * choicesRightWrap = new QVBoxLayout();
                choicesRow->addLayout(choicesRightWrap);
                {
                    QHBoxLayout * choicesCentering = new QHBoxLayout();
                    choicesCentering->addStretch();
                    choicesCentering->addWidget(m_impl->rightChoice);
                    choicesCentering->addStretch();
                    choicesRightWrap->addLayout(choicesCentering);
                }
                choicesRightWrap->addWidget(m_impl->rightChoiceDescr);
            }
            choicesRow->addStretch();
        }
    }
    {
        QHBoxLayout * bottomButtons = new QHBoxLayout();
        bottomButtons->addStretch();
        m_impl->closeButton = DialogUtils::makeAcceptButton(this);
        bottomButtons->addWidget(m_impl->closeButton);
        mainLayout->addLayout(bottomButtons);
        m_impl->closeButton->setFocusPolicy(Qt::NoFocus);
    }
    m_impl->choiceBtns   = {{m_impl->leftChoice, m_impl->rightChoice}};
    m_impl->choiceDescrs = {{m_impl->leftChoiceDescr, m_impl->rightChoiceDescr}};
//    for (auto * lbl : {m_impl->title, m_impl->levelAndClass, m_impl->statPrimaryTxt}) {
//        lbl->setMinimumWidth(this->sizeHint().width())
//    }
    DialogUtils::setupClickSound(m_impl->leftChoice);
    DialogUtils::setupClickSound(m_impl->rightChoice);
    connect(m_impl->leftChoice, &QPushButton::clicked , this, [this]{m_impl->m_currentChoice = 0; m_impl->closeButton->setEnabled(true); });
    connect(m_impl->rightChoice, &QPushButton::clicked, this, [this]{m_impl->m_currentChoice = 1; m_impl->closeButton->setEnabled(true); });
    for (auto *btn : m_impl->choiceBtns)
        connect(btn, &FlatButton::doubleClicked, this, &QDialog::accept);
}

void HeroLevelupDialog::setInfo(const HeroLevelupDialog::LevelUpDecision& info)
{
    m_impl->title->setText(tr("%1 is getting to new level.").arg(info.heroHame));
    m_impl->expLeft->setPixmap(info.expIcon);
    m_impl->expRight->setPixmap(info.expIcon);
    m_impl->portrait->setPixmap(info.heroPortrait);
    m_impl->levelAndClass->setText(tr("%1 now at the level %2, %3").arg(info.heroHame).arg(info.level).arg(info.heroClass));
    m_impl->statPrimaryIcon->setPixmap(info.primaryStatIcon);
    m_impl->statPrimaryTxt->setText(tr("%1 +1").arg(info.primaryStatName));
    m_impl->orText->setVisible(info.choices.size() == 2);
    auto applyData = [this](int index, const Choice & choice) {
        FlatButton * btn = m_impl->choiceBtns[index];
        btn->setIcon(choice.icon);
        btn->setVisible(true);
        QLabel * lbl = m_impl->choiceDescrs[index];
        lbl->setText(QString("%1<br>%2").arg(choice.skillLevelName).arg(choice.skillName));
        lbl->setVisible(true);
    };
    auto hideData = [this](int index) {
        FlatButton * btn = m_impl->choiceBtns[index];
        btn->setVisible(false);
        QLabel * lbl = m_impl->choiceDescrs[index];
        lbl->setVisible(false);
    };
    if (info.choices.size() == 2) {
        m_impl->choiceDescription->setText(tr("Your hero could learn<br> %1 %2 <br>or %3 %4.")
                                            .arg(info.choices[0].skillLevelName).arg(info.choices[0].skillName)
                                            .arg(info.choices[1].skillLevelName).arg(info.choices[1].skillName));
        applyData(0, info.choices[0]);
        applyData(1, info.choices[1]);
        m_impl->closeButton->setEnabled(false);
        this->setFocus();
    } else if (info.choices.size() == 1) {
        m_impl->choiceDescription->setText(tr("Your hero learning<br> %1 %2.")
                                            .arg(info.choices[0].skillLevelName).arg(info.choices[0].skillName));
        applyData(0, info.choices[0]);
        hideData(1);
        m_impl->closeButton->setEnabled(true);
        m_impl->m_currentChoice = 0;
        m_impl->leftChoice->setFocus();
    } else {
        m_impl->choiceDescription->setText("");
        hideData(0);
        hideData(1);
        m_impl->closeButton->setEnabled(true);
        m_impl->m_currentChoice = 0;
    }
}

void HeroLevelupDialog::accept()
{
    emit accepted();

}

void HeroLevelupDialog::reject()
{
    emit rejected();
}

int HeroLevelupDialog::getChoiceIndex() const
{
    return m_impl->m_currentChoice;
}

HeroLevelupDialog::~HeroLevelupDialog() = default;


}
