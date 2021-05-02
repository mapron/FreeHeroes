/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureModels.hpp"

#include "LibraryModels.hpp"

#include "LibraryWrappersMetatype.hpp"

#include "AdventureStack.hpp"
#include "AdventureSquad.hpp"
#include "AdventureHero.hpp"

#include "ResizePixmap.hpp"

namespace FreeHeroes::Gui {
using namespace Core;

HeroBagEditModel::HeroBagEditModel(ArtifactsModel* artifacts, Core::AdventureHeroMutablePtr source, QObject* parent)
    : QAbstractTableModel(parent)
    , m_artifacts(artifacts)
    , m_source(source)
{
    m_filter = new ArtifactsFilterModel(this);
    m_filter->setSourceModel(m_artifacts);
    m_filter->setFilterIndex(-1);
}

HeroBagEditModel::~HeroBagEditModel() = default;

void HeroBagEditModel::refresh()
{
    emit dataChanged(this->index(0, 1), this->index(this->rowCount() - 1, 1));
}

int HeroBagEditModel::rowCount(const QModelIndex&) const
{
    return m_filter->rowCount();
}

int HeroBagEditModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant HeroBagEditModel::data(const QModelIndex& index, int role) const
{
    if (index.column() == 0) {
        QVariant srcData = m_filter->data(m_filter->index(index.row(), index.column()), role);
        if (role == Qt::DecorationRole) {
            return resizePixmap(srcData.value<QPixmap>(), QSize(33, 33), true);
        }
        return srcData;
    }

    if (index.column() == 1) {
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole) {
            auto art = m_filter->data(m_filter->index(index.row(), 0), ArtifactsModel::SourceObject).value<Core::LibraryArtifactConstPtr>();
            if (role == Qt::UserRole)
                return QVariant::fromValue(art);

            return m_source->getBagCount(art);
        }
    }

    return QVariant();
}

Qt::ItemFlags HeroBagEditModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == 1)
        f |= Qt::ItemIsEditable;
    return f;
}

bool HeroBagEditModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.column() == 1 && role == Qt::EditRole) {
        auto art = m_filter->data(m_filter->index(index.row(), 0), ArtifactsModel::SourceObject).value<Core::LibraryArtifactConstPtr>();

        m_source->artifactsBag[art] = value.toInt();
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

QVariant HeroBagEditModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return QStringList{ tr("Artifact"), tr("Count") }.value(section);
    }
    return QVariant();
}

struct HeroWearingEditModel::Impl {
    struct SlotInfo {
        ArtifactSlotType compatSlot;
        ArtifactSlotType wearingSlot;
        QString          name;
    };
    QList<SlotInfo> m_slots;
};

HeroWearingEditModel::HeroWearingEditModel(ArtifactsModel* artifacts, Core::AdventureHeroMutablePtr source, QObject* parent)
    : QAbstractTableModel(parent)
    , m_artifacts(artifacts)
    , m_source(source)
    , m_impl(std::make_unique<Impl>())
{
    // clang-format off
    m_impl->m_slots = {
        {   ArtifactSlotType::Sword  ,  ArtifactSlotType::Sword   , tr("Sword:") },
        {   ArtifactSlotType::Shield ,  ArtifactSlotType::Shield  , tr("Shield:") },
        {   ArtifactSlotType::Helm   ,  ArtifactSlotType::Helm    , tr("Helm:") },
        {   ArtifactSlotType::Torso  ,  ArtifactSlotType::Torso   , tr("Torso:") },
        {   ArtifactSlotType::Ring   ,  ArtifactSlotType::Ring    , tr("Ring 1:") },
        {   ArtifactSlotType::Ring   ,  ArtifactSlotType::Ring1   , tr("Ring 2:") },
        {   ArtifactSlotType::Neck   ,  ArtifactSlotType::Neck    , tr("Neck:") },
        {   ArtifactSlotType::Boots  ,  ArtifactSlotType::Boots   , tr("Boots:") },
        {   ArtifactSlotType::Cape   ,  ArtifactSlotType::Cape    , tr("Cape:") },
        {   ArtifactSlotType::Misc   ,  ArtifactSlotType::Misc    , tr("Misc 1:") },
        {   ArtifactSlotType::Misc   ,  ArtifactSlotType::Misc1   , tr("Misc 2:") },
        {   ArtifactSlotType::Misc   ,  ArtifactSlotType::Misc2   , tr("Misc 3:") },
        {   ArtifactSlotType::Misc   ,  ArtifactSlotType::Misc3   , tr("Misc 4:") },
        {   ArtifactSlotType::Misc   ,  ArtifactSlotType::Misc4   , tr("Misc 5:") },
        {   ArtifactSlotType::BmShoot,  ArtifactSlotType::BmShoot , tr("Shooting machine:") },
        {   ArtifactSlotType::BmAmmo ,  ArtifactSlotType::BmAmmo  , tr("Ammo cart:") },
        {   ArtifactSlotType::BmTent ,  ArtifactSlotType::BmTent  , tr("Tent:") },
    };
    // clang-format on
}

HeroWearingEditModel::~HeroWearingEditModel() = default;
void HeroWearingEditModel::refresh()
{
    emit dataChanged(this->index(0, 1), this->index(this->rowCount() - 1, 1));
}

int HeroWearingEditModel::rowCount(const QModelIndex&) const
{
    return m_impl->m_slots.size();
}

int HeroWearingEditModel::columnCount(const QModelIndex&) const
{
    return 2;
}

QVariant HeroWearingEditModel::data(const QModelIndex& index, int role) const
{
    if (index.column() == 0 && role == Qt::DisplayRole) {
        int r = index.row();
        return m_impl->m_slots.value(r).name;
    }
    if (index.column() == 1 && (role == Qt::DisplayRole || role == Qt::DecorationRole || role == Qt::UserRole || role == Qt::UserRole + 1)) {
        int         r           = index.row();
        const auto& slotInfo    = m_impl->m_slots[r];
        auto        wearingSlot = slotInfo.wearingSlot;
        auto        wearingArt  = m_source->getArtifact(wearingSlot);
        if (role == Qt::DisplayRole) {
            if (!wearingArt)
                return tr("-- Empty -- ");
            return m_artifacts->find(wearingArt)->getName();
        } else if (role == Qt::DecorationRole) {
            if (!wearingArt)
                return QVariant();
            return resizePixmap(m_artifacts->find(wearingArt)->getIconStash(), QSize(33, 33), true);
        } else if (role == Qt::UserRole) {
            return static_cast<int>(slotInfo.compatSlot);
        } else if (role == Qt::UserRole + 1) {
            return QVariant::fromValue(wearingArt);
        }
    }
    return {};
}

Qt::ItemFlags HeroWearingEditModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == 1)
        f |= Qt::ItemIsEditable;
    return f;
}

bool HeroWearingEditModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::UserRole + 1 && index.column() == 1) {
        int         r                      = index.row();
        const auto& slotInfo               = m_impl->m_slots[r];
        auto        wearingSlot            = slotInfo.wearingSlot;
        auto        wearingArt             = value.value<LibraryArtifactConstPtr>();
        m_source->artifactsOn[wearingSlot] = wearingArt;
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

QVariant HeroWearingEditModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return QStringList{ tr("Slot"), tr("Artifact") }.value(section);
    }
    return QVariant();
}

HeroSkillsEditModel::HeroSkillsEditModel(SkillsModel* skillsModel, AdventureHeroMutablePtr source, QObject* parent)
    : QAbstractTableModel(parent)
    , m_skills(skillsModel)
    , m_source(source)
{
}

HeroSkillsEditModel::~HeroSkillsEditModel() = default;

void HeroSkillsEditModel::refresh()
{
    emit dataChanged(this->index(0, 0), this->index(this->rowCount() - 1, 1));
}

int HeroSkillsEditModel::rowCount(const QModelIndex& parent) const
{
    return 8; // @todo: bring come config value here!
}

int HeroSkillsEditModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant HeroSkillsEditModel::data(const QModelIndex& index, int role) const
{
    int                           r     = index.row();
    int                           level = -1;
    LibrarySecondarySkillConstPtr skill = nullptr;
    if (r < (int) m_source->secondarySkills.size()) {
        skill = m_source->secondarySkills[r].skill;
        level = m_source->secondarySkills[r].level;
    }

    if (index.column() == 0) {
        if (skill && level >= 0) {
            auto guiSkill = m_skills->find(skill);
            if (role == Qt::DecorationRole) {
                return guiSkill->getIconSmall(level);
            }
            if (role == Qt::DisplayRole) {
                return guiSkill->getName();
            }
            if (role == SkillsModel::SourceObject) {
                return QVariant::fromValue(skill);
            }
            if (role == SkillsModel::GuiObject) {
                return QVariant::fromValue(guiSkill);
            }
        }

        return QVariant();
    }
    if (index.column() == 1) {
        if (role == Qt::DisplayRole) {
            return GuiSkill::getSkillLevelName(level);
        }
        if (role == Qt::EditRole) {
            return level;
        }
    }
    return QVariant();
}

Qt::ItemFlags HeroSkillsEditModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags                 f     = Qt::ItemIsEnabled;
    int                           r     = index.row();
    LibrarySecondarySkillConstPtr skill = nullptr;
    if (r < (int) m_source->secondarySkills.size()) {
        skill = m_source->secondarySkills[r].skill;
    }
    if (index.column() == 0 || (index.column() == 1 && skill))
        f |= Qt::ItemIsEditable;
    return f;
}

bool HeroSkillsEditModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.column() == 0 && role == SkillsModel::SourceObject) {
        auto skill = value.value<LibrarySecondarySkillConstPtr>();
        int  r     = index.row();
        if (r < (int) m_source->secondarySkills.size()) {
            m_source->secondarySkills[r].skill = skill;
            if (!skill) {
                m_source->secondarySkills.erase(m_source->secondarySkills.begin() + r);
            }
        } else if (skill) {
            if (m_source->getSkillLevel(skill) != -1) // @todo: the best is just exclude skill from select list.
                return false;
            m_source->secondarySkills.push_back({ skill, 0 });
        }

        refresh();
        return true;
    }
    if (index.column() == 1 && role == Qt::EditRole) {
        int r                              = index.row();
        m_source->secondarySkills[r].level = value.toInt();
        emit dataChanged(this->index(index.row(), 0), index);
        return true;
    }
    return false;
}

QVariant HeroSkillsEditModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return QStringList{ tr("Skill"), tr("Level") }.value(section);
    }
    return QVariant();
}

HeroSpellbookEditModel::HeroSpellbookEditModel(SpellsModel* spellsModel, AdventureHeroMutablePtr source, QObject* parent)
    : QAbstractListModel(parent)
    , m_spellsModel(spellsModel)
    , m_source(source)
{
    m_filter = new SpellsFilterModel(this);
    m_filter->setSourceModel(spellsModel);
}
HeroSpellbookEditModel::~HeroSpellbookEditModel() = default;

void HeroSpellbookEditModel::refresh()
{
    emit dataChanged(this->index(0, 0), this->index(this->rowCount() - 1, 0));
}

int HeroSpellbookEditModel::rowCount(const QModelIndex&) const
{
    return m_filter->rowCount();
}

QVariant HeroSpellbookEditModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::CheckStateRole) {
        auto spell = m_filter->data(m_filter->index(index.row(), index.column()), SpellsModel::SourceObject).value<LibrarySpellConstPtr>();
        return m_source->spellbook.contains(spell) ? Qt::Checked : Qt::Unchecked;
    }
    QVariant filterData = m_filter->data(m_filter->index(index.row(), index.column()), role);
    if (role == Qt::DecorationRole && filterData.isValid()) {
        return resizePixmap(filterData.value<QPixmap>(), QSize(33, 33), true);
    }
    return filterData;
}

Qt::ItemFlags HeroSpellbookEditModel::flags(const QModelIndex&) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    return f;
}

bool HeroSpellbookEditModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::CheckStateRole) {
        auto spell = m_filter->data(m_filter->index(index.row(), index.column()), SpellsModel::SourceObject).value<LibrarySpellConstPtr>();
        if (spell) {
            if (value == Qt::Checked)
                m_source->spellbook.insert(spell);
            else
                m_source->spellbook.erase(spell);
        }
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

}
