/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleResultDialog.hpp"

#include "DependencyInjector.hpp"
#include "DialogUtils.hpp"
#include "CustomFrames.hpp"
#include "FormatUtils.hpp"
#include "UiCommonModel.hpp"

#include <QPushButton>
#include <QBoxLayout>
#include <QLabel>
#include <QMovie>

namespace FreeHeroes::Gui {

struct BattleResultDialog::Impl {
    const LibraryModelsProvider* m_modelsProvider;
    struct SideWidgets {
        DarkFrameLabel*   portrait = nullptr;
        DarkFrameLabel*   name     = nullptr;
        DarkFrameLabel*   status   = nullptr;
        CasualtiesWidget* loss     = nullptr;
    };
    SideWidgets              sides[2];
    QLabel*                  animation         = nullptr;
    QMovie*                  animationMovie    = nullptr;
    DarkFrameLabel*          resultDescription = nullptr;
    Sound::ISoundResourcePtr m_music;

    Impl(const LibraryModelsProvider* modelsProvider)
        : m_modelsProvider(modelsProvider)
    {}
};

BattleResultDialog::BattleResultDialog(const LibraryModelsProvider* modelsProvider, QWidget* parent)
    : QDialog(parent)
    , m_impl(std::make_unique<Impl>(modelsProvider))

{
    DialogUtils::commonDialogSetup(this);

    auto makeLabel = [this](Qt::Alignment align, int lines) -> DarkFrameLabel* {
        auto* lbl = new DarkFrameLabel(this);
        lbl->setAlignment(align | Qt::AlignVCenter);
        lbl->setMargin(5);
        int h = lbl->fontMetrics().height();
        lbl->setMinimumHeight(lines * h + 5);
        return lbl;
    };
    auto enlargeLabel = [](QLabel* lbl, int size, bool bold) {
        QFont f = lbl->font();
        f.setPointSize(f.pointSize() + size);
        f.setBold(bold);
        lbl->setFont(f);
    };

    const int minimumNameWidth = 90;

    QVBoxLayout* mainLayout = DialogUtils::makeMainDialogFrame(this);
    mainLayout->setContentsMargins(10, 15, 10, 10);
    {
        QHBoxLayout* portraitsAndAnim = new QHBoxLayout();
        portraitsAndAnim->setMargin(0);
        portraitsAndAnim->setSpacing(3);
        mainLayout->addLayout(portraitsAndAnim);
        {
            QVBoxLayout* leftPortrait = new QVBoxLayout();
            leftPortrait->setMargin(0);
            leftPortrait->setSpacing(0);
            portraitsAndAnim->addLayout(leftPortrait);
            {
                QHBoxLayout* leftPortraitTop = new QHBoxLayout();
                leftPortrait->addLayout(leftPortraitTop);
                m_impl->sides[0].portrait = new DarkFrameLabel(this);
                leftPortraitTop->addWidget(m_impl->sides[0].portrait);
                leftPortraitTop->addStretch();
            }
            portraitsAndAnim->addSpacing(2);
            {
                m_impl->sides[0].status = makeLabel(Qt::AlignLeft, 1);
                m_impl->sides[0].status->setMinimumWidth(minimumNameWidth);
                leftPortrait->addWidget(m_impl->sides[0].status);
            }
            leftPortrait->addStretch();
        }
        {
            QVBoxLayout* labelsAndAnim = new QVBoxLayout();
            labelsAndAnim->setMargin(0);
            labelsAndAnim->setSpacing(10);
            portraitsAndAnim->addLayout(labelsAndAnim);
            {
                QHBoxLayout* centerTop = new QHBoxLayout();
                centerTop->setMargin(0);
                centerTop->setSpacing(10);
                labelsAndAnim->addLayout(centerTop);
                m_impl->sides[0].name = makeLabel(Qt::AlignLeft, 1);
                m_impl->sides[1].name = makeLabel(Qt::AlignRight, 1);
                centerTop->addWidget(m_impl->sides[0].name, 1);
                centerTop->addWidget(m_impl->sides[1].name, 1);
            }
            {
                const QSize animSize(256, 120);
                m_impl->animation = new QLabel(this);
                m_impl->animation->setFrameStyle(QFrame::NoFrame);
                labelsAndAnim->addWidget(m_impl->animation);
                m_impl->animation->setFixedSize(animSize);
            }
        }
        {
            QVBoxLayout* rightPortrait = new QVBoxLayout();
            portraitsAndAnim->addLayout(rightPortrait);
            {
                QHBoxLayout* rightPortraitTop = new QHBoxLayout();
                rightPortrait->addLayout(rightPortraitTop);
                m_impl->sides[1].portrait = new DarkFrameLabel(this);
                rightPortraitTop->addStretch();
                rightPortraitTop->addWidget(m_impl->sides[1].portrait);
            }
            {
                m_impl->sides[1].status = makeLabel(Qt::AlignRight, 1);
                m_impl->sides[1].status->setMinimumWidth(minimumNameWidth);
                rightPortrait->addWidget(m_impl->sides[1].status);
            }
            rightPortrait->addStretch();
        }
    }
    QVBoxLayout* center = new QVBoxLayout();
    center->setContentsMargins(20, 0, 20, 20);
    mainLayout->addLayout(center);
    {
        QHBoxLayout* centerResult = new QHBoxLayout();
        centerResult->setContentsMargins(20, 5, 20, 5);
        center->addLayout(centerResult);

        m_impl->resultDescription = makeLabel(Qt::AlignCenter, 4);
        m_impl->resultDescription->setMargin(15);
        centerResult->addWidget(m_impl->resultDescription);
    }
    {
        QVBoxLayout* centerCasualties = new QVBoxLayout();
        center->addLayout(centerCasualties);
        QString text     = FormatUtils::prepareDescription(tr("{Battlefield casualties}"));
        auto*   casTitle = new QLabel(text, this);
        enlargeLabel(casTitle, 3, true);
        casTitle->setAlignment(Qt::AlignCenter);
        centerCasualties->addWidget(casTitle);

        auto* casTitleAtt = new QLabel(tr("Attacker"), this);
        enlargeLabel(casTitleAtt, 2, true);
        casTitleAtt->setAlignment(Qt::AlignCenter);
        centerCasualties->addWidget(casTitleAtt);
        m_impl->sides[0].loss = new CasualtiesWidget(this);
        centerCasualties->addWidget(m_impl->sides[0].loss);

        centerCasualties->addSpacing(15);

        auto* casTitleDef = new QLabel(tr("Defender"), this);
        enlargeLabel(casTitleDef, 3, true);
        casTitleDef->setAlignment(Qt::AlignCenter);
        centerCasualties->addWidget(casTitleDef);

        m_impl->sides[1].loss = new CasualtiesWidget(this);
        centerCasualties->addWidget(m_impl->sides[1].loss);
    }
    {
        QHBoxLayout* bottomButtons = new QHBoxLayout();
        mainLayout->addLayout(bottomButtons);
        bottomButtons->addWidget(DialogUtils::makeRejectButton(this));
        auto* resultQuestion = new QLabel(tr("Save battle results?"), this);
        enlargeLabel(resultQuestion, 2, false);
        resultQuestion->setAlignment(Qt::AlignCenter);
        bottomButtons->addWidget(resultQuestion, 1);
        bottomButtons->addWidget(DialogUtils::makeAcceptButton(this));
    }
}

void BattleResultDialog::setResultInfo(const BattleResultDialog::ResultInfo& info)
{
    for (int i = 0; i < 2; i++) {
        m_impl->sides[i].portrait->setPixmap(info.sides[i].portrait);
        m_impl->sides[i].status->setText(info.sides[i].win ? tr("Winner") : tr("Loser"));
        m_impl->sides[i].name->setText(info.sides[i].name);
        m_impl->sides[i].loss->setResultInfo(info.sides[i].loss);
    }

    if (m_impl->animationMovie)
        m_impl->animationMovie->deleteLater();
    auto ui = m_impl->m_modelsProvider->ui();

    auto animPtr           = (info.goodResult ? ui->win.anim : ui->lose.anim);
    m_impl->animationMovie = animPtr->exists() ? animPtr->create(this) : nullptr;
    m_impl->animation->setMovie(m_impl->animationMovie);
    m_impl->m_music = info.goodResult ? ui->win.music : ui->lose.music;
    m_impl->resultDescription->setText(info.resultDescription);
}

void BattleResultDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    if (m_impl->animationMovie)
        m_impl->animationMovie->start();
    m_impl->m_music->play();
}

BattleResultDialog::~BattleResultDialog() = default;

}
