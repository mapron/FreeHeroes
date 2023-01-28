/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TemplatePlayerWidget.hpp"

#include "ui_TemplatePlayerWidget.h"

#include "LibraryModels.hpp"
#include "LibraryEditorModels.hpp"

#include "LibraryWrappersMetatype.hpp"

namespace FreeHeroes {
using namespace Gui;

namespace {

void setHeroToCombo(Core::LibraryHeroConstPtr hero, QComboBox* combo)
{
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i, HeroesModel::SourceObject).value<HeroesModel::SrcTypePtr>() == hero) {
            combo->setCurrentIndex(i);
            return;
        }
    }
    combo->setCurrentIndex(-1);
}

void setFactionToCombo(Core::LibraryFactionConstPtr faction, QComboBox* combo)
{
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i, FactionsModel::SourceObject).value<FactionsModel::SrcTypePtr>() == faction) {
            combo->setCurrentIndex(i);
            break;
        }
    }
}

void setPolicyToCombo(FHRngUserSettings::HeroGeneration policy, QComboBox* combo)
{
    combo->setCurrentIndex(static_cast<int>(policy));
}

Core::LibraryHeroConstPtr getHeroFromCombo(QComboBox* combo)
{
    return combo->currentData(HeroesModel::SourceObject).value<Core::LibraryHeroConstPtr>();
}

Core::LibraryFactionConstPtr getFactionFromCombo(QComboBox* combo)
{
    return combo->currentData(FactionsModel::SourceObject).value<Core::LibraryFactionConstPtr>();
}

FHRngUserSettings::HeroGeneration getPolicyFromCombo(QComboBox* combo)
{
    return static_cast<FHRngUserSettings::HeroGeneration>(combo->currentIndex());
}

class HeroFilterModel : public QSortFilterProxyModel {
    QComboBox* m_factionCombo;
    bool       m_useFactionFilter{ true };

public:
    HeroFilterModel(QComboBox* factionCombo, QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        m_factionCombo = factionCombo;
        setDynamicSortFilter(false);
    }

    void setUseFactionFilter(bool flag)
    {
        if (m_useFactionFilter == flag)
            return;
        m_useFactionFilter = flag;
        invalidate();
    }

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
        auto        hero   = index0.data(HeroesModel::SourceObject).value<HeroesModel::SrcTypePtr>();
        if (!hero)
            return false;

        if (m_useFactionFilter) {
            Core::LibraryFactionConstPtr filterFaction = getFactionFromCombo(m_factionCombo);
            if (filterFaction) {
                return filterFaction == hero->faction;
            }
        }

        return true;
    }
};

}

TemplatePlayerWidget::TemplatePlayerWidget(const Gui::LibraryModelsProvider* modelsProvider, QWidget* parent)
    : QFrame(parent)
    , m_ui(std::make_unique<Ui::TemplatePlayerWidget>())
{
    m_ui->setupUi(this);

    m_ui->comboBoxHeroMain->setIconSize({ 32, 24 });
    m_ui->comboBoxHeroExtra->setIconSize({ 32, 24 });

    auto* heroFilterModelMain  = new HeroFilterModel(m_ui->comboBoxFaction, this);
    auto* heroFilterModelExtra = new HeroFilterModel(m_ui->comboBoxFaction, this);

    auto* comboModel = new ComboModel("", { 32, 24 }, modelsProvider->heroes(), this);

    heroFilterModelMain->setSourceModel(comboModel);
    heroFilterModelExtra->setSourceModel(comboModel);

    m_ui->comboBoxHeroMain->setModel(heroFilterModelMain);
    m_ui->comboBoxHeroExtra->setModel(heroFilterModelExtra);

    FactionsFilterModel* factionsFilter = new FactionsFilterModel(false, this);
    factionsFilter->setSourceModel(modelsProvider->factions());

    FactionsComboModel* factionsCombo = new FactionsComboModel(factionsFilter, this);
    m_ui->comboBoxFaction->setModel(factionsCombo);

    connect(m_ui->comboBoxFaction, qOverload<int>(&QComboBox::currentIndexChanged), this, [heroFilterModelMain, heroFilterModelExtra, this](int) {
        heroFilterModelMain->invalidate();
        heroFilterModelExtra->invalidate();

        if (m_ui->comboBoxHeroMainPolicy->currentIndex() >= 3 && m_ui->comboBoxHeroMain->currentIndex() == -1)
            m_ui->comboBoxHeroMain->setCurrentIndex(0);
        if (m_ui->comboBoxHeroExtraPolicy->currentIndex() >= 3 && m_ui->comboBoxHeroExtra->currentIndex() == -1)
            m_ui->comboBoxHeroExtra->setCurrentIndex(0);
    });
    connect(m_ui->comboBoxHeroMain, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && m_ui->comboBoxHeroMainPolicy->currentIndex() < 3)
            m_ui->comboBoxHeroMainPolicy->setCurrentIndex(3);
    });
    connect(m_ui->comboBoxHeroExtra, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && m_ui->comboBoxHeroExtraPolicy->currentIndex() < 3)
            m_ui->comboBoxHeroExtraPolicy->setCurrentIndex(3);
    });

    connect(m_ui->comboBoxHeroMainPolicy, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, heroFilterModelMain](int index) {
        if (index < 3)
            m_ui->comboBoxHeroMain->setCurrentIndex(-1);
        if (index >= 3 && m_ui->comboBoxHeroMain->currentIndex() == -1)
            m_ui->comboBoxHeroMain->setCurrentIndex(0);

        heroFilterModelMain->setUseFactionFilter(index == 4);
    });
    connect(m_ui->comboBoxHeroExtraPolicy, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, heroFilterModelExtra](int index) {
        if (index < 3)
            m_ui->comboBoxHeroExtra->setCurrentIndex(-1);
        if (index >= 3 && m_ui->comboBoxHeroExtra->currentIndex() == -1)
            m_ui->comboBoxHeroExtra->setCurrentIndex(0);

        heroFilterModelExtra->setUseFactionFilter(index == 4);
    });
}

TemplatePlayerWidget::~TemplatePlayerWidget()
{
}

void TemplatePlayerWidget::setConfig(const FHRngUserSettings::UserPlayer& settings)
{
    setFactionToCombo(settings.m_faction, m_ui->comboBoxFaction);

    setPolicyToCombo(settings.m_startingHeroGen, m_ui->comboBoxHeroMainPolicy);
    setPolicyToCombo(settings.m_extraHeroGen, m_ui->comboBoxHeroExtraPolicy);

    setHeroToCombo(settings.m_startingHero, m_ui->comboBoxHeroMain);
    setHeroToCombo(settings.m_extraHero, m_ui->comboBoxHeroExtra);
}

FHRngUserSettings::UserPlayer TemplatePlayerWidget::getConfig() const
{
    FHRngUserSettings::UserPlayer result;
    result.m_faction = getFactionFromCombo(m_ui->comboBoxFaction);

    result.m_startingHero = getHeroFromCombo(m_ui->comboBoxHeroMain);
    result.m_extraHero    = getHeroFromCombo(m_ui->comboBoxHeroExtra);

    result.m_startingHeroGen = getPolicyFromCombo(m_ui->comboBoxHeroMainPolicy);
    result.m_extraHeroGen    = getPolicyFromCombo(m_ui->comboBoxHeroExtraPolicy);

    if (result.m_startingHeroGen != FHRngUserSettings::HeroGeneration::FixedStarting
        && result.m_startingHeroGen != FHRngUserSettings::HeroGeneration::FixedAny)
        result.m_startingHero = nullptr;
    if (result.m_extraHeroGen != FHRngUserSettings::HeroGeneration::FixedStarting
        && result.m_extraHeroGen != FHRngUserSettings::HeroGeneration::FixedAny)
        result.m_extraHero = nullptr;
    return result;
}

void TemplatePlayerWidget::setPlayerColorText(QString txt)
{
    m_ui->labelColor->setText(txt);
}

}
