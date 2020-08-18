/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "EmulatorMainWidget.hpp"

#include "ui_EmulatorMainWidget.h"

// Gui
#include "BattleWidget.hpp"
#include "SettingsWidget.hpp"
#include "DependencyInjector.hpp"
#include "BattleResultDialog.hpp"
#include "HeroLevelupDialog.hpp"
#include "GeneralPopupDialog.hpp"
#include "ReplayFileManager.hpp"

// Gui - wrappers
#include "UiCommonModel.hpp"
#include "AdventureWrappers.hpp"
#include "LibraryWrappers.hpp"
#include "LibraryModels.hpp"
#include "LibraryEditorModels.hpp"
#include "LibraryWrappersMetatype.hpp"

// Gui - interface
#include "IGraphicsLibrary.hpp"
#include "IMusicBox.hpp"
#include "FsUtilsQt.hpp"

// Core
#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "AI.hpp"
#include "BattleManager.hpp"
#include "AdventureReplay.hpp"
#include "AdventureEstimation.hpp"
#include "GeneralEstimation.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryMapObject.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryGameRules.hpp"
#include "AdventureKingdom.hpp"

// Platform
#include "Profiler.hpp"
#include "Logger.hpp"

#include <QInputDialog>
/// @todo some  AppLocationManager for paths??
#include <QStandardPaths>
#include <QSet>

namespace FreeHeroes::BattleEmulator {

using namespace Core;
using namespace Gui;


EmulatorMainWidget::EmulatorMainWidget(IGraphicsLibrary & graphicsLibrary,
                                       ICursorLibrary & cursorLibrary,
                                       IGameDatabase & gameDatabase,
                                       IRandomGeneratorFactory & randomGeneratorFactory,
                                       Sound::IMusicBox & musicBox,
                                       IAppSettings & appSettings,
                                       LibraryModelsProvider & modelsProvider,
                                       QWidget* parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::EmulatorMainWidget>())
    , m_graphicsLibrary(graphicsLibrary)
    , m_cursorLibrary(cursorLibrary)
    , m_gameDatabase(gameDatabase)
    , m_randomGeneratorFactory(randomGeneratorFactory)
    , m_musicBox(musicBox)
    , m_appSettings(appSettings)
    , m_modelsProvider(modelsProvider)
    , m_adventureState(std::make_unique<AdventureState>())
    , m_adventureStatePrev(std::make_unique<AdventureState>())
    , m_adventureKingdom(std::make_unique<AdventureKingdom>())
{
    ProfilerScope scope("EmulatorMainWidget()");
    storeDependency(this, modelsProvider);
    storeDependency(this, cursorLibrary);
    storeDependency(this, musicBox);
    storeDependency(this, appSettings);

    m_uiRng = m_randomGeneratorFactory.create();
    m_uiRng->makeGoodSeed();

    {
        auto rng = m_randomGeneratorFactory.create();
        rng->makeGoodSeed();
        m_adventureState->m_seed = rng->getSeed();
    }

    m_ui->setupUi(this);

    ProfilerScope scope1("EmulatorMainWidget() - after setupUI");

    connect(m_ui->pushButtonStart, &QPushButton::clicked, this, &EmulatorMainWidget::startBattle);
    connect(m_ui->pushButtonSettings, &QPushButton::clicked, this, &EmulatorMainWidget::showSettings);
    connect(m_ui->pushButtonReplayBattle, &QPushButton::clicked, this, &EmulatorMainWidget::startReplay);
    connect(m_ui->pushButtonReplayLoadAdv, &QPushButton::clicked, this, &EmulatorMainWidget::loadAdventureData);


    m_adventureState->m_att.squad.stacks.resize(m_gameDatabase.gameRules()->limits.stacks);
    m_adventureState->m_def.squad.stacks.resize(m_gameDatabase.gameRules()->limits.stacks);

    m_ui->armyConfigAtt->setModels(modelsProvider, m_uiRng.get());
    m_ui->armyConfigDef->setModels(modelsProvider, m_uiRng.get());

    m_guiAdventureArmyAtt = std::make_unique<GuiAdventureArmy>(*modelsProvider.units(), *modelsProvider.heroes(), &m_adventureState->m_att);
    m_guiAdventureArmyDef = std::make_unique<GuiAdventureArmy>(*modelsProvider.units(), *modelsProvider.heroes(), &m_adventureState->m_def);

    m_ui->armyConfigAtt->setSource(m_guiAdventureArmyAtt.get());
    m_ui->armyConfigDef->setSource(m_guiAdventureArmyDef.get());
    m_ui->armyConfigAtt->initHero();
    m_ui->armyConfigDef->setAIControl(true);

    auto * terrainsFilter = new TerrainsFilterModel(this);
    terrainsFilter->setSourceModel(modelsProvider.terrains());
    m_ui->comboBoxTerrain->setModel(new TerrainsComboModel(terrainsFilter, this));
    m_ui->comboBoxTerrain->setIconSize({24, 24});
    for (int i = 0; i < m_ui->comboBoxTerrain->count(); ++i) {
        auto terrain = m_ui->comboBoxTerrain->itemData(i, TerrainsModel::SourceObject).value<Core::LibraryTerrainConstPtr>();
        if (terrain && terrain->id == "sod.terrain.grass") {
            m_ui->comboBoxTerrain->setCurrentIndex(i);
            break;
        }
    }
    connect(m_ui->comboBoxTerrain, qOverload<int>(&QComboBox::currentIndexChanged), this, &EmulatorMainWidget::onTerrainChanged);
    auto * comboModel = new MapObjectsComboModel(modelsProvider.mapObjects(), this);
    auto * mapObjectsTree = new MapObjectsTreeModel(comboModel, this);
    m_ui->comboBoxObjectPreset->setModel(mapObjectsTree);
    m_ui->comboBoxObjectPreset->setIconSize({32, 24});
    m_ui->comboBoxObjectPreset->expandAll();


    connect(m_ui->comboBoxObjectPreset, qOverload<int>(&QComboBox::currentIndexChanged), this, &EmulatorMainWidget::onObjectPresetChanged);


    const QList<QPair<QString, QString>> obstaclePresets {
        {tr("Empty field"), ""},
        {"Test 1 - ) (", "4;2 - 5;3 - 5;4 - 5;5 - 10;5 - 9;6 - 9;7 - 9;8"},
        {"Test 2 - ^ O", "2;1 - 3;1 - 8;4 - 9;4 - 10;4 - 11;4 - 9;5 - 10;5 - 11;5 - 12;5"},
        {"Test 3 - horiz", "2;5 - 3;5 - 4;5 - 5;5 - 6;5 - 7;5 - 8;5 - 9;5 - 9;6 - 10;6 - 11;7"},
        {"Test 4 - vert", "4;2 - 5;2 - 6;3 - 6;4 - 7;5 - 7;6 - 8;7 - 8;8 - 9;8 - 8;9"},
    };
    for (auto preset : obstaclePresets)
        m_ui->comboBoxObstaclePreset->addItem(preset.first, preset.second);

    m_ui->comboBoxPositionsPreset->addItems({
                                               tr("Standard"),
                                                tr("Map object common"),
                                                tr("Churchyard 1"),
                                                tr("Churchyard 2"),
                                                tr("Ruins"),
                                                tr("Spit")
                                            });


    connect(m_guiAdventureArmyAtt.get(), &GuiAdventureArmy::dataChanged, this, [this]{onAttDataChanged(); });
    connect(m_guiAdventureArmyDef.get(), &GuiAdventureArmy::dataChanged, this, [this]{onDefDataChanged(); });

    connect(m_ui->armyConfigAtt, &ArmyConfigWidget::makeLevelup, this, [this](int newLevel){
        m_adventureState->m_att.hero.experience = GeneralEstimation(m_gameDatabase.gameRules()).getExperienceForLevel(newLevel);
        checkForHeroLevelUps();
    });
    connect(m_ui->armyConfigDef, &ArmyConfigWidget::makeLevelup, this, [this](int newLevel){
        m_adventureState->m_def.hero.experience = GeneralEstimation(m_gameDatabase.gameRules()).getExperienceForLevel(newLevel);
        checkForHeroLevelUps();
    });

    connect(m_ui->pushButtonNextDay  , &QPushButton::clicked, this, &EmulatorMainWidget::makeNewDay);
    connect(m_ui->pushButtonRegenMana, &QPushButton::clicked, this, &EmulatorMainWidget::makeManaRegen);

    connect(m_ui->pushButtonClearReplays, &QPushButton::clicked, this, [this]{
        m_replayManager->clearAll();
    });
    connect(m_ui->pushButtonRenameReplay, &QPushButton::clicked, this, [this]{
        if (m_ui->comboBoxReplaySelect->currentIndex() < 0)
            return;
        QString s = QInputDialog::getText(this, tr("Renaming replay"), tr("Enter new name:"), QLineEdit::Normal, m_replayManager->m_records[m_ui->comboBoxReplaySelect->currentIndex()].displayName);
        m_replayManager->renameCurrent(m_ui->comboBoxReplaySelect->currentIndex(), s);
    });

    m_adventureStatePrev.reset();

    onTerrainChanged();

    m_replayManager = std::make_unique<ReplayFileManager>();
    m_replayManager->setRoot(QString2stdPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation)) / "Replays");
    m_replayManager->load();

    m_ui->comboBoxReplaySelect->setModel(m_replayManager.get());
    m_ui->comboBoxReplaySelect->setCurrentIndex(m_ui->comboBoxReplaySelect->count() - 1);

    m_ui->comboBoxObjectPreset->selectIndex(mapObjectsTree->index(0, 0, mapObjectsTree->index(1, 0, {})));


    setWindowTitle(tr("Battle emulator"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

EmulatorMainWidget::~EmulatorMainWidget() = default;

void EmulatorMainWidget::startBattle()
{
    readAdventureStateFromUI();
    execBattle(false, m_ui->checkBoxQuickCombat->isChecked());
}

void EmulatorMainWidget::startReplay()
{
    if (m_ui->comboBoxReplaySelect->currentIndex() < 0)
        return;
    execBattle(true, false);
}

void EmulatorMainWidget::showSettings()
{
    m_appSettings.showSettingsEditor(this);
}

void EmulatorMainWidget::show()
{
    QWidget::show();
    m_musicBox.musicPrepare(Sound::IMusicBox::MusicSettings{
                              Sound::IMusicBox::MusicSet::Intro}.setDelay(1000).setFadeIn(3000))->play();
}

void EmulatorMainWidget::onAttDataChanged(bool forcedUpdate)
{
    if (!forcedUpdate && m_adventureStatePrev && m_adventureStatePrev->m_att.isEqualTo(m_adventureState->m_att))
        return;
    if (!m_adventureStatePrev)
        m_adventureStatePrev = std::make_unique<AdventureState>();

    m_adventureStatePrev->m_att = m_adventureState->m_att;

    AdventureEstimation(m_gameDatabase.gameRules()).calculateArmy(m_adventureState->m_att, m_adventureState->m_terrain);

    m_adventureKingdom->dayIncome  = m_adventureState->m_att.estimated.dayIncome;
    m_adventureKingdom->weekIncome = m_adventureState->m_att.estimated.weekIncomeMax;
    m_ui->kingdomStatusWidget->update(*m_adventureKingdom);

    m_ui->kingdomStatusWidget->setResourceIcons(m_modelsProvider.ui()->resourceIconsSmall);

    m_ui->armyConfigAtt->refresh();
}

void EmulatorMainWidget::onDefDataChanged(bool forcedUpdate)
{
    if (!forcedUpdate && m_adventureStatePrev && m_adventureStatePrev->m_def.isEqualTo(m_adventureState->m_def))
        return;
    if (!m_adventureStatePrev)
        m_adventureStatePrev = std::make_unique<AdventureState>();

    m_adventureStatePrev->m_def = m_adventureState->m_def;

    AdventureEstimation(m_gameDatabase.gameRules()).calculateArmy(m_adventureState->m_def, m_adventureState->m_terrain);

    m_ui->armyConfigDef->refresh();
}

void EmulatorMainWidget::makeNewDay()
{
    if (m_adventureState->m_att.hasHero()) {
        AdventureEstimation(m_gameDatabase.gameRules()).calculateDayStart(m_adventureState->m_att.hero);
        m_ui->armyConfigAtt->refresh();
        onAttDataChanged(true);
    }
    if (m_adventureState->m_def.hasHero()) {
        AdventureEstimation(m_gameDatabase.gameRules()).calculateDayStart(m_adventureState->m_def.hero);
        m_ui->armyConfigDef->refresh();
        onDefDataChanged(true);
    }
    if (!m_adventureStatePrev)
        m_adventureStatePrev = std::make_unique<AdventureState>();

    m_adventureKingdom->currentDate.next();
    m_adventureKingdom->currentResources += m_adventureKingdom->dayIncome;
    if (m_adventureKingdom->currentDate.weekStart)
        m_adventureKingdom->currentResources += m_adventureKingdom->weekIncome;
    m_ui->kingdomStatusWidget->update(*m_adventureKingdom);

    *m_adventureStatePrev = *m_adventureState;

    m_musicBox.effectPrepare({"newday"})->play();
}

void EmulatorMainWidget::makeManaRegen()
{
    bool playEffect = false;
    if (m_adventureState->m_att.hasHero()) {
        playEffect = true;
        m_adventureState->m_att.hero.mana = m_adventureState->m_att.hero.estimated.maxMana;
        m_ui->armyConfigAtt->refresh();
        onAttDataChanged(true);
    }
    if (m_adventureState->m_def.hasHero()) {
        playEffect = true;
        m_adventureState->m_def.hero.mana = m_adventureState->m_def.hero.estimated.maxMana;
        m_ui->armyConfigDef->refresh();
        onDefDataChanged(true);
    }
    if (playEffect)
        m_musicBox.effectPrepare({"treasure"})->play();
}

void EmulatorMainWidget::onTerrainChanged()
{
    auto terrain = m_ui->comboBoxTerrain->currentData(TerrainsModel::SourceObject).value<TerrainsModel::SrcTypePtr>();
    assert(terrain);
    m_adventureState->m_terrain = terrain;
    onAttDataChanged(true);
    onDefDataChanged(true);
}

void EmulatorMainWidget::onObjectPresetChanged()
{
    m_adventureState->m_mapObject = nullptr;
    auto mapObject = m_ui->comboBoxObjectPreset->currentData(MapObjectsModel::SourceObject).value<MapObjectsModel::SrcTypePtr>();
    if (!mapObject) {
        m_ui->comboBoxPositionsPreset->setCurrentIndex(0);
        return;
    }

    m_adventureState->m_mapObjectVariant = m_ui->comboBoxObjectPreset->currentIndex();
    m_adventureState->m_mapObject = mapObject;

    const int layoutIndex = static_cast<int>(m_adventureState->m_mapObject->fieldLayout);
    m_ui->comboBoxPositionsPreset->setCurrentIndex(layoutIndex);
    m_ui->armyConfigDef->initFromMapObject(m_adventureState->m_mapObject, m_adventureState->m_mapObjectVariant);
}

void EmulatorMainWidget::readAdventureStateFromUI()
{
    const BattleFieldGeometry defaultField { 15, 11 };

    std::vector<BattlePosition> obstacles;
    QStringList obstacleStr = m_ui->comboBoxObstaclePreset->currentData().toString().split(" - ", Qt::SkipEmptyParts);
    for (auto part : obstacleStr)
        obstacles.push_back(BattlePosition{part.split(";").value(0).toInt(), part.split(";").value(1).toInt()});

    const BattleFieldPreset fieldPreset {
        obstacles,
        defaultField,
        static_cast<FieldLayout>(m_ui->comboBoxPositionsPreset->currentIndex())
    };
    m_adventureState->m_field = fieldPreset;

}

void EmulatorMainWidget::loadAdventureData()
{
    ReplayFileManager::Record replayRec;
    replayRec = m_replayManager->m_records[m_ui->comboBoxReplaySelect->currentIndex()];
    AdventureReplayData replayData;
    replayData.load(replayRec.battleReplay, m_gameDatabase);
    *m_adventureState = replayData.m_adv;
    m_adventureStatePrev.reset();
    m_guiAdventureArmyAtt->updateGuiState();
    m_guiAdventureArmyDef->updateGuiState();

    m_guiAdventureArmyAtt->emitChanges();
    m_guiAdventureArmyDef->emitChanges();

    m_ui->comboBoxPositionsPreset->setCurrentIndex(static_cast<int>(replayData.m_adv.m_field.layout));
}

void EmulatorMainWidget::applyCurrentObjectRewards(QString defenderName)
{
    QStringList rewardDescriptionTitles;
    GeneralPopupDialog::Items items;
    {
        int rewIndex = m_adventureState->m_mapObject->variants[m_adventureState->m_mapObjectVariant].rewardIndex;
        const auto & reward = m_adventureState->m_mapObject->rewards[rewIndex];
        assert(reward.totalItems() > 0);
        const bool isSingleReward = reward.totalItems() == 1;



        auto rng = m_randomGeneratorFactory.create();
        rng->makeGoodSeed();

        for (auto  & artReward : reward.artifacts.artifacts) {
            auto treasureClass = artReward.treasureClass;
            std::deque<LibraryArtifactConstPtr> suggestions;
            for (auto art : m_gameDatabase.artifacts()->records()) {
                if (art->treasureClass == treasureClass)
                    suggestions.push_back(art);
            }
            for (int i = 0; i < artReward.count; i++) {
                auto ind = rng->gen(suggestions.size() - 1);
                auto art = suggestions[ind];
                suggestions.erase(suggestions.begin() + ind);
                m_adventureState->m_att.hero.artifactsBag[art]++;
                auto name = m_modelsProvider.artifacts()->find(art)->getName();
                auto iconId = art->presentationParams.iconStash;
                auto pix = m_graphicsLibrary.getPixmap(iconId)->get();
                items << GeneralPopupDialog::Item{pix, name, true};
                rewardDescriptionTitles << name;
            }
        }

        const auto resourceRewards = ResourceAmountHelper().trasformResourceAmount(reward.resources);
        for (auto  & resReward : resourceRewards) {
            rewardDescriptionTitles << QString("%1 %2").arg(resReward.amount).arg(resReward.name);
            auto res = m_gameDatabase.resources()->find(resReward.id.toStdString());
            auto iconId = isSingleReward ? res->presentationParams.iconLarge : res->presentationParams.icon;
            auto pix = m_graphicsLibrary.getPixmap(iconId)->get();
            items << GeneralPopupDialog::Item{pix, QString("%1").arg(resReward.amount), false};
        }
        if (reward.unit.unit) {
            m_adventureState->m_att.squad.addWithMerge(reward.unit.unit, reward.unit.count);
            m_guiAdventureArmyAtt->getSquad()->updateGuiState();
            m_guiAdventureArmyAtt->getSquad()->emitChanges();
            auto name = m_modelsProvider.units()->find(reward.unit.unit)->getNameWithCount(reward.unit.count);
            rewardDescriptionTitles << name;
            auto iconId = reward.unit.unit->presentationParams.portrait;
            auto pix = m_graphicsLibrary.getPixmap(iconId)->get();
            items << GeneralPopupDialog::Item{pix, name, true};
            onAttDataChanged(true);
        }
        m_adventureKingdom->currentResources += reward.resources;
    }

    QString rewardDescriptionTitlesJoined;
    if (!rewardDescriptionTitles.empty())
    {
        QString last = rewardDescriptionTitles.takeLast();
        if (rewardDescriptionTitles.empty())
            rewardDescriptionTitlesJoined = last;
        else
            rewardDescriptionTitlesJoined = rewardDescriptionTitles.join(", ") + tr(" and ") + last;
    }

    const QString rewardDescription = tr("%1 is no problem anymore, and %2 now in your hands.").arg(defenderName).arg(rewardDescriptionTitlesJoined);

    GeneralPopupDialog dlg(rewardDescription, items, true, false, this);
    dlg.adjustSize();
    dlg.exec();

    m_ui->kingdomStatusWidget->update(*m_adventureKingdom);
}

int EmulatorMainWidget::execBattle(bool isReplay, bool isQuick)
{
    AdventureReplayData replayData;
    replayData.m_adv = *m_adventureState;
    ReplayFileManager::Record replayRec;
    if (isReplay)
    {
        replayRec = m_replayManager->m_records[m_ui->comboBoxReplaySelect->currentIndex()];
        replayData.load(replayRec.battleReplay, m_gameDatabase);
        AdventureEstimation(m_gameDatabase.gameRules()).calculateArmy(replayData.m_adv.m_att, replayData.m_adv.m_terrain);
        AdventureEstimation(m_gameDatabase.gameRules()).calculateArmy(replayData.m_adv.m_def, replayData.m_adv.m_terrain);
    }
    else
    {
        replayRec = m_replayManager->makeNewUnique();
    }

    BattleArmy att(&replayData.m_adv.m_att, BattleStack::Side::Attacker);
    BattleArmy def(&replayData.m_adv.m_def, BattleStack::Side::Defender);

    if (att.isEmpty() || def.isEmpty())
        return QDialog::Rejected;

    auto rng = m_randomGeneratorFactory.create();
    rng->setSeed(replayData.m_adv.m_seed);

    BattleManager battle(att, def,
                         replayData.m_adv.m_field,
                         rng,
                         m_gameDatabase.gameRules());
    IBattleView * battleView = &battle;
    IBattleControl * battleControl= &battle;
    IAIFactory * aiFactory = &battle;

    if (!isQuick)
        QTimer::singleShot(300, this, [ &battle]{
            battle.start();
        });


    std::unique_ptr<BattleReplayPlayer> player;
    std::unique_ptr<BattleReplayRecorder> recorder;
    if (isReplay)
    {

        player = std::make_unique<BattleReplayPlayer>(*battleControl, replayData.m_bat);
        //
    }
    else if (m_ui->checkBoxEnableRecording->isChecked())
    {
        recorder = std::make_unique<BattleReplayRecorder>(*battleControl, replayData.m_bat);
        battleControl = recorder.get();
    }

    std::unique_ptr<BattleWidget> battleWidget;
    if (!isQuick) {
        battleWidget = std::make_unique<BattleWidget>(*battleView,
                   *battleControl, *aiFactory, m_modelsProvider,
                   replayData.m_adv.m_field.field,

                   m_appSettings, this);
        if (isReplay)
            battleWidget->setReplay(player.get());
        else
            battleWidget->setAI(m_ui->armyConfigAtt->isAIControl(), m_ui->armyConfigDef->isAIControl());

        auto terrain = replayData.m_adv.m_terrain;
        assert(terrain);
        QPixmap back = m_graphicsLibrary.getPixmap(terrain->presentationParams.backgroundsBattle[0])->get();
        battleWidget->setBackground(back);
    }
    if (!isReplay) {
        replayData.save(replayRec.battleReplay);
        m_replayManager->add(replayRec);
        m_ui->comboBoxReplaySelect->setCurrentIndex(m_ui->comboBoxReplaySelect->count() - 1);
    }


    int result = QDialog::Accepted;
    if (!isQuick)
        result = battleWidget->exec();

    if (isQuick) {
        battle.start();
        int limitStepCheck = 1000;
        Logger(Logger::Notice) << "Starting quick combat.";
        IAI::AIParams params;
        auto ai = battle.makeAI(params, *battleControl);
        limitStepCheck = ai->run(limitStepCheck);
        if (!limitStepCheck) {
             GeneralPopupDialog::messageBox(tr("AI reached step limit: that probably an error."), this);
             return 0;
        }
        Logger(Logger::Info) << ai->getProfiling();

        Logger(Logger::Notice) << "Quick combat ended, " << limitStepCheck << " steps are remain.";
    }

    if (!isReplay)
    {
        if (m_ui->checkBoxEnableRecording->isChecked()) {
            replayData.save(replayRec.battleReplay);
        }

        if (result == QDialog::Accepted) {

            auto getCasualties = [this](BattleSquad::LossInformation & batInfo) -> CasualtiesWidget::Info {
                CasualtiesWidget::Info result;

                result.totalHP = batInfo.totalHpLoss;
                result.totalValue = batInfo.totalValueLoss;
                for (auto & unitLoss : batInfo.units) {
                    CasualtiesWidget::LossInfo widgetInfo;
                    widgetInfo.count = unitLoss.loss;
                    widgetInfo.portrait = m_modelsProvider.units()->find(unitLoss.unit)->getPortraitSmall();
                    widgetInfo.isDead  = unitLoss.isDead;
                    result.units.push_front(widgetInfo);
                }
                return result;
            };

            BattleResultDialog resultDialog(m_modelsProvider, this);
            BattleResultDialog::ResultInfo resultInfo;
            const bool attackerWin = att.hasAlive();
            const bool defenderWin = def.hasAlive();
            resultInfo.goodResult = attackerWin;
            std::array<BattleArmy*,2> armies{{&att, &def}};
            std::array<bool, 2> isWin{{attackerWin, defenderWin}};
            for (int i=0; i< 2; i++) {
                BattleArmy& army = *armies[i];
                auto & side = resultInfo.sides[i];
                auto batInfo = army.squad->estimateLoss();
                auto heroUI = m_modelsProvider.heroes()->find(army.adventure->hero.library);
                auto unitUI = m_modelsProvider.units()->find(batInfo.strongestUnit);

                QString heroName = army.adventure->hasHero() ? heroUI->getName() : "";
                QString unitName = batInfo.strongestUnit ? unitUI->getName() : "";
                side.name = heroName.isEmpty() ? unitName : heroName;
                side.portrait = army.adventure->hasHero() ? heroUI->getPortraitLarge() : QPixmap();
                if (side.portrait.isNull() && batInfo.strongestUnit)
                    side.portrait = unitUI->getPortraitLarge();
                side.win = isWin[i];

                side.loss = getCasualties(batInfo);
            }
            int defenderHPloss = resultInfo.sides[1].loss.totalHP;
            resultInfo.resultDescription = resultInfo.goodResult ?
                                           tr("Glorious victory!<br>For the valor in the battle, %1 recieves %2 experience").arg(resultInfo.sides[0].name).arg(defenderHPloss) :
                                            tr("Your forces have suffered a crushing defeat, and %1 now leaves you").arg(resultInfo.sides[0].name);
            resultDialog.setResultInfo(resultInfo);

            int userAcceptResult = resultDialog.exec();

            if (userAcceptResult == QDialog::Accepted) {
                auto attLoss = att.squad->estimateLoss();
                auto defLoss = def.squad->estimateLoss();  // both loss should be estimated before updateAdventure
                att.updateAdventure(m_adventureState->m_att, defLoss, attackerWin);
                def.updateAdventure(m_adventureState->m_def, attLoss, defenderWin);
                checkForHeroLevelUps(false);
                m_adventureStatePrev.reset();
                m_guiAdventureArmyAtt->updateGuiState();
                m_guiAdventureArmyDef->updateGuiState();

                m_guiAdventureArmyAtt->emitChanges();
                m_guiAdventureArmyDef->emitChanges();

                if (m_adventureState->m_mapObject && m_adventureState->m_mapObject->rewards.size() && resultInfo.goodResult ) {
                    applyCurrentObjectRewards(resultInfo.sides[1].name);
                }
            }
        }
    }
    m_musicBox.musicPrepare(Sound::IMusicBox::MusicSet::Intro)->play();
    return result;
}

void EmulatorMainWidget::checkForHeroLevelUps(bool fromDebugWidget)
{
    if (m_adventureState->m_att.hasHero()) {
        AdventureEstimation::LevelUpResult result;
        auto & hero  = m_adventureState->m_att.hero;
        auto guiAdvHero = m_guiAdventureArmyAtt->getHero();

        HeroLevelupDialog::LevelUpDecision decision;
        decision.heroHame = guiAdvHero->getName();
        decision.heroClass = guiAdvHero->getClassName();
        decision.heroPortrait = guiAdvHero->getGuiHero()->getPortraitLarge();
        decision.expIcon = m_modelsProvider.ui()->skillInfo[HeroPrimaryParamType::Experience].iconLarge->get();
        auto rng = m_randomGeneratorFactory.create();
        rng->makeGoodSeed();
        HeroLevelupDialog dlg(this);
        AdventureEstimation estimation(m_gameDatabase.gameRules());

        while ((result = estimation.calculateHeroLevelUp(hero, *rng)).isValid()) {
            decision.choices.clear();
            const auto & statInfo = m_modelsProvider.ui()->skillInfo[result.primarySkillUpdated];
            decision.primaryStatName = statInfo.name;
            decision.primaryStatIcon = statInfo.iconMedium->get();
            decision.level = result.newLevel;

            for (size_t i = 0; i < result.choices.size(); ++i) {
                auto choice = result.choices[i];
                int skillLevel = result.choicesLevels[i];
                HeroLevelupDialog::Choice guiChoice;
                auto guiSkill = m_modelsProvider.skills()->find(choice);
                guiChoice.icon = guiSkill->getIconMedium(skillLevel);
                guiChoice.skillLevelName = GuiSkill::getSkillLevelName(skillLevel);
                guiChoice.skillName = guiSkill->getName();
                decision.choices << guiChoice;
            }


            dlg.setInfo(decision);
            dlg.show();
            m_musicBox.effectPrepare({"nwherolv"})->play();

            QEventLoop loop;
            connect(&dlg, &QDialog::accepted, &loop, &QEventLoop::quit);
            connect(&dlg, &QDialog::rejected, &loop, &QEventLoop::quit);
            loop.exec();

            const int index = dlg.getChoiceIndex();
            LibrarySecondarySkillConstPtr choice = nullptr;
            if (index >= 0 && result.choices.size() > 0)
                choice = result.choices[index];

            estimation.applyLevelUpChoice(m_adventureState->m_att.hero, choice);

            estimation.calculateArmy(m_adventureState->m_att, m_adventureState->m_terrain);
        }

    }
    if (fromDebugWidget) {
        onAttDataChanged(true);
        onDefDataChanged(true);
    }
}

}
