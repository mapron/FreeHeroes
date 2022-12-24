/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibraryModels.hpp"

#include "LibraryWrappersMetatype.hpp"
#include "IGraphicsLibrary.hpp"
#include "ResizePixmap.hpp"
#include "UiCommonModel.hpp"

#include "IGameDatabase.hpp"

#include "LibraryResource.hpp"

#include <deque>
#include <unordered_map>

namespace FreeHeroes::Gui {
using namespace Core;

template<typename WrapperType>
struct AbstractGuiWrapperListModel<WrapperType>::Impl {
    using SrcTypePtr = typename AbstractGuiWrapperListModel<WrapperType>::SrcTypePtr;
    std::deque<WrapperType>                 m_records;
    std::unordered_map<SrcTypePtr, size_t>  m_index;
    std::unordered_map<std::string, size_t> m_indexById;
};

template<typename T>
AbstractGuiWrapperListModel<T>::AbstractGuiWrapperListModel(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, QObject* parent)
    : QAbstractListModel(parent)
    , m_impl(std::make_unique<Impl>())
    , m_graphicsLibrary(graphicsLibrary)
    , m_musicBox(musicBox)
{
}
template<typename T>
AbstractGuiWrapperListModel<T>::~AbstractGuiWrapperListModel() = default;

template<typename WrapperType>
void AbstractGuiWrapperListModel<WrapperType>::clear()
{
    beginResetModel();
    m_impl->m_records.clear();
    m_impl->m_index.clear();
    m_impl->m_indexById.clear();
    endResetModel();
}

template<typename WrapperType>
void AbstractGuiWrapperListModel<WrapperType>::addRecord(SrcTypePtr record)
{
    m_impl->m_records.emplace_back(m_musicBox, m_graphicsLibrary, record);
    m_impl->m_index[record]         = m_impl->m_records.size() - 1;
    m_impl->m_indexById[record->id] = m_impl->m_records.size() - 1;
}
template<typename WrapperType>
typename AbstractGuiWrapperListModel<WrapperType>::WrapperTypePtr
AbstractGuiWrapperListModel<WrapperType>::find(SrcTypePtr record) const
{
    auto it = m_impl->m_index.find(record);
    if (it == m_impl->m_index.cend())
        return nullptr;

    return &m_impl->m_records[it->second];
}

template<typename WrapperType>
typename AbstractGuiWrapperListModel<WrapperType>::WrapperTypePtr
AbstractGuiWrapperListModel<WrapperType>::find(const std::string& id) const
{
    auto it = m_impl->m_indexById.find(id);
    if (it == m_impl->m_indexById.cend())
        return nullptr;

    return &m_impl->m_records[it->second];
}

template<typename WrapperType>
QVariant AbstractGuiWrapperListModel<WrapperType>::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const int r = index.row();
    if (role == Qt::DisplayRole) {
        return m_impl->m_records[r].getName();
    }
    if (role == GuiObject) {
        return QVariant::fromValue(WrapperTypePtr(&m_impl->m_records[r]));
    }
    if (role == SourceObject) {
        return QVariant::fromValue(SrcTypePtr(m_impl->m_records[r].getSource()));
    }
    return QVariant();
}

template<typename WrapperType>
int AbstractGuiWrapperListModel<WrapperType>::rowCount(const QModelIndex&) const
{
    return m_impl->m_records.size();
}

const std::string ArtifactsModel::s_catapult  = "sod.artifact.battleMachine.catapult";
const std::string ArtifactsModel::s_spellbook = "sod.artifact.special.spellbook";

ArtifactsModel::ArtifactsModel(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, QObject* parent)
    : Base(musicBox, graphicsLibrary, parent)
{
    m_lock = graphicsLibrary->getPixmapByKey({ "sod.sprites.artifactsInventory", 0, 145 });
}

QVariant ArtifactsModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole || role == IconSmall || role == IconMedium || role == IconLarge) {
        auto    artifact = Base::data(index, GuiObject).value<GuiArtifactConstPtr>();
        QPixmap img      = artifact->getIconStash();
        return QVariant(img);
    }
    return Base::data(index, role);
}

ArtifactsFilterModel::ArtifactsFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);
}

void ArtifactsFilterModel::setFilterIndex(int filter)
{
    m_filter = filter;
    invalidateFilter();
}

bool ArtifactsFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    auto        rec    = index0.data(ArtifactsModel::SourceObject).value<ArtifactsModel::SrcTypePtr>();
    if (!rec)
        return false;

    if (m_filter == -1 && rec->slot != Core::ArtifactSlotType::BagOnly) {
        if (rec->treasureClass == Core::LibraryArtifact::TreasureClass::Special || rec->treasureClass == Core::LibraryArtifact::TreasureClass::BattleMachine)
            return false;
    }
    if (m_filter >= 0) {
        if (static_cast<int>(rec->slot) != m_filter)
            return false;
        if (rec->treasureClass == Core::LibraryArtifact::TreasureClass::Special)
            return false;
    }

    return true;
}

QVariant UnitsModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole || role == IconSmall) {
        auto    unit = Base::data(index, GuiObject).value<GuiUnitConstPtr>();
        QPixmap img  = unit->getPortraitSmall();
        return QVariant(img);
    } else if (role == IconMedium || role == IconLarge) {
        auto    unit = Base::data(index, GuiObject).value<GuiUnitConstPtr>();
        QPixmap img  = unit->getPortraitLarge();
        return QVariant(img);
    }

    return Base::data(index, role);
}

UnitsFilterModel::UnitsFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool UnitsFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    auto        rec    = index0.data(UnitsModel::SourceObject).value<UnitsModel::SrcTypePtr>();
    if (!rec)
        return false;

    if (rec->faction->alignment == LibraryFaction::Alignment::Special)
        return false;

    return true;
}

QVariant HeroesModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole || role == IconSmall) {
        auto    hero = Base::data(index, GuiObject).value<GuiHeroConstPtr>();
        QPixmap img  = hero->getPortraitSmall();
        return QVariant(img);
    } else if (role == IconMedium || role == IconLarge) {
        auto    hero = Base::data(index, GuiObject).value<GuiHeroConstPtr>();
        QPixmap img  = hero->getPortraitLarge();
        return QVariant(img);
    }

    return Base::data(index, role);
}

QVariant SkillsModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole || role == IconSmall) {
        auto    skill = Base::data(index, GuiObject).value<GuiSkillConstPtr>();
        QPixmap img   = skill->getIconSmall(2);
        return QVariant(img);
    }

    return Base::data(index, role);
}

QVariant SpellsModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole || role == IconSmall) {
        auto    spell = Base::data(index, GuiObject).value<GuiSpellConstPtr>();
        QPixmap img   = spell->getIconInt();
        return QVariant(img);
    }

    return Base::data(index, role);
}

SpellsFilterModel::SpellsFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool SpellsFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    auto        spell  = index0.data(SpellsModel::SourceObject).value<SpellsModel::SrcTypePtr>();
    if (!spell)
        return false;

    return spell->isTeachable;
}

QVariant FactionsModel::data(const QModelIndex& index, int role) const
{
    return Base::data(index, role);
}

FactionsFilterModel::FactionsFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool FactionsFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index0  = sourceModel()->index(sourceRow, 0, sourceParent);
    auto        faction = index0.data(FactionsModel::SourceObject).value<FactionsModel::SrcTypePtr>();
    if (!faction)
        return false;

    return faction->alignment != LibraryFaction::Alignment::Special;
}

QVariant TerrainsModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole || role == IconSmall) {
        auto    terrain = Base::data(index, GuiObject).value<GuiTerrainConstPtr>();
        QPixmap img     = terrain->getIcon();
        return QVariant(img);
    }
    return Base::data(index, role);
}

TerrainsFilterModel::TerrainsFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool TerrainsFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index0  = sourceModel()->index(sourceRow, 0, sourceParent);
    auto        terrain = index0.data(TerrainsModel::SourceObject).value<TerrainsModel::SrcTypePtr>();
    if (!terrain)
        return false;

    return !terrain->isObstacle;
}

QVariant MapBanksModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole || role == IconSmall) {
        auto    mapBank = Base::data(index, GuiObject).value<GuiMapBankConstPtr>();
        QPixmap img     = mapBank->getIcon();
        return QVariant(img);
    }
    if (role == VaraintNames) {
        auto mapBank = Base::data(index, GuiObject).value<GuiMapBankConstPtr>();
        return QVariant(mapBank->getVariantNames());
    }
    return Base::data(index, role);
}

LibraryModelsProvider::LibraryModelsProvider(const IGameDatabase*    gameDatabase,
                                             Sound::IMusicBox*       musicBox,
                                             const IGraphicsLibrary* graphicsLibrary,
                                             QObject*                parent)
    : QObject(parent)
{
    m_artifacts = new ArtifactsModel(musicBox, graphicsLibrary, this);
    m_units     = new UnitsModel(musicBox, graphicsLibrary, this);
    m_heroes    = new HeroesModel(musicBox, graphicsLibrary, this);
    m_skills    = new SkillsModel(musicBox, graphicsLibrary, this);
    m_spells    = new SpellsModel(musicBox, graphicsLibrary, this);
    m_factions  = new FactionsModel(musicBox, graphicsLibrary, this);
    m_terrains  = new TerrainsModel(musicBox, graphicsLibrary, this);
    m_mapBanks  = new MapBanksModel(musicBox, graphicsLibrary, this);
    m_uiCommon  = new UiCommonModel(musicBox, graphicsLibrary, this);

    for (auto* rec : gameDatabase->artifacts()->records())
        m_artifacts->addRecord(rec);
    for (auto* rec : gameDatabase->units()->records())
        m_units->addRecord(rec);
    for (auto* rec : gameDatabase->heroes()->records())
        m_heroes->addRecord(rec);
    for (auto* rec : gameDatabase->secSkills()->records())
        m_skills->addRecord(rec);
    for (auto* rec : gameDatabase->spells()->records())
        m_spells->addRecord(rec);
    for (auto* rec : gameDatabase->factions()->records())
        m_factions->addRecord(rec);
    for (auto* rec : gameDatabase->terrains()->records())
        m_terrains->addRecord(rec);
    for (auto* rec : gameDatabase->mapBanks()->records())
        m_mapBanks->addRecord(rec);

    QMap<QString, QPixmap> resourceIcons;
    for (auto* res : gameDatabase->resources()->records()) {
        auto pix                                                        = graphicsLibrary->getPixmap(res->presentationParams.icon)->get();
        m_uiCommon->resourceIcons[QString::fromStdString(res->id)]      = pix;
        m_uiCommon->resourceIconsSmall[QString::fromStdString(res->id)] = resizePixmap(pix, { 24, 24 });
    }
}

template class AbstractGuiWrapperListModel<GuiArtifact>;
template class AbstractGuiWrapperListModel<GuiUnit>;
template class AbstractGuiWrapperListModel<GuiHero>;
template class AbstractGuiWrapperListModel<GuiSkill>;
template class AbstractGuiWrapperListModel<GuiSpell>;
template class AbstractGuiWrapperListModel<GuiFaction>;
template class AbstractGuiWrapperListModel<GuiTerrain>;
template class AbstractGuiWrapperListModel<GuiMapBank>;

}
