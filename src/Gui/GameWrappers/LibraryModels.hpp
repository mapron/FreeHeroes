/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiGameWrappersExport.hpp"

#include "LibraryWrappers.hpp"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QConcatenateTablesProxyModel>

#include <memory>

namespace FreeHeroes::Core {
class IGameDatabase;
}

namespace FreeHeroes::Gui {

template<typename WrapperTypeT>
class GUIGAMEWRAPPERS_EXPORT AbstractGuiWrapperListModel : public QAbstractListModel {
public:
    using WrapperType    = WrapperTypeT;
    using SrcType        = typename WrapperType::SrcType;
    using WrapperTypePtr = const WrapperType*;
    using SrcTypePtr     = const SrcType*;

    AbstractGuiWrapperListModel(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, QObject* parent);
    ~AbstractGuiWrapperListModel();

    void           clear();
    void           addRecord(SrcTypePtr record);
    WrapperTypePtr find(SrcTypePtr source) const;
    WrapperTypePtr find(const std::string& id) const;

    QVariant data(const QModelIndex& index, int role) const override;
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;

    enum ItemDataRole
    {
        GuiObject = Qt::UserRole + 1, // WrapperTypePtr
        SourceObject,                 // SrcTypePtr

        IconSmall,
        IconMedium,
        IconLarge,

        LastBaseRole
    };

private:
    struct Impl;
    std::unique_ptr<Impl>   m_impl;
    const IGraphicsLibrary* m_graphicsLibrary;
    Sound::IMusicBox*       m_musicBox;
};

class GUIGAMEWRAPPERS_EXPORT ArtifactsModel : public AbstractGuiWrapperListModel<GuiArtifact> {
public:
    using Base = AbstractGuiWrapperListModel<GuiArtifact>;
    ArtifactsModel(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, QObject* parent);

    QVariant data(const QModelIndex& index, int role) const override;

    GuiArtifactConstPtr findCatapult() const { return this->find(s_catapult); }
    GuiArtifactConstPtr findSpellbook() const { return this->find(s_spellbook); }
    QPixmap             getLockIcon() const { return m_lock->get(); }

private:
    static const std::string s_catapult;
    static const std::string s_spellbook;
    IAsyncPixmapPtr          m_lock;
};

class GUIGAMEWRAPPERS_EXPORT ArtifactsFilterModel : public QSortFilterProxyModel {
public:
    ArtifactsFilterModel(QObject* parent);

    void setFilterIndex(int filter);

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int m_filter = -1;
};

class GUIGAMEWRAPPERS_EXPORT UnitsModel : public AbstractGuiWrapperListModel<GuiUnit>
    , public GuiUnitProvider {
public:
    using Base = AbstractGuiWrapperListModel<GuiUnit>;
    using AbstractGuiWrapperListModel::AbstractGuiWrapperListModel;

    QVariant        data(const QModelIndex& index, int role) const override;
    GuiUnitConstPtr find(Core::LibraryUnitConstPtr source) const override
    {
        return Base::find(source);
    }
    using Base::find;
};

class GUIGAMEWRAPPERS_EXPORT UnitsFilterModel : public QSortFilterProxyModel {
public:
    UnitsFilterModel(QObject* parent);

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};

class GUIGAMEWRAPPERS_EXPORT HeroesModel : public AbstractGuiWrapperListModel<GuiHero>
    , public GuiHeroProvider {
public:
    using Base = AbstractGuiWrapperListModel<GuiHero>;
    using AbstractGuiWrapperListModel::AbstractGuiWrapperListModel;

    QVariant        data(const QModelIndex& index, int role) const override;
    GuiHeroConstPtr find(Core::LibraryHeroConstPtr source) const override
    {
        return Base::find(source);
    }
    using Base::find;
};

class GUIGAMEWRAPPERS_EXPORT SkillsModel : public AbstractGuiWrapperListModel<GuiSkill>
    , public GuiSkillProvider {
public:
    using Base = AbstractGuiWrapperListModel<GuiSkill>;
    using AbstractGuiWrapperListModel::AbstractGuiWrapperListModel;

    QVariant         data(const QModelIndex& index, int role) const override;
    GuiSkillConstPtr find(Core::LibrarySecondarySkillConstPtr source) const override
    {
        return Base::find(source);
    }
    using Base::find;
};

class GUIGAMEWRAPPERS_EXPORT SpellsModel : public AbstractGuiWrapperListModel<GuiSpell>
    , public GuiSpellProvider {
public:
    using Base = AbstractGuiWrapperListModel<GuiSpell>;
    using AbstractGuiWrapperListModel::AbstractGuiWrapperListModel;

    QVariant         data(const QModelIndex& index, int role) const override;
    GuiSpellConstPtr find(Core::LibrarySpellConstPtr source) const override
    {
        return Base::find(source);
    }
    using Base::find;
};

class GUIGAMEWRAPPERS_EXPORT SpellsFilterModel : public QSortFilterProxyModel {
public:
    SpellsFilterModel(QObject* parent);

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};

class GUIGAMEWRAPPERS_EXPORT FactionsModel : public AbstractGuiWrapperListModel<GuiFaction> {
public:
    using Base = AbstractGuiWrapperListModel<GuiFaction>;
    using AbstractGuiWrapperListModel::AbstractGuiWrapperListModel;

    QVariant data(const QModelIndex& index, int role) const override;
};

class GUIGAMEWRAPPERS_EXPORT FactionsFilterModel : public QSortFilterProxyModel {
public:
    FactionsFilterModel(QObject* parent);

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};

class GUIGAMEWRAPPERS_EXPORT TerrainsModel : public AbstractGuiWrapperListModel<GuiTerrain> {
public:
    using Base = AbstractGuiWrapperListModel<GuiTerrain>;
    using AbstractGuiWrapperListModel::AbstractGuiWrapperListModel;

    QVariant data(const QModelIndex& index, int role) const override;
};

class GUIGAMEWRAPPERS_EXPORT TerrainsFilterModel : public QSortFilterProxyModel {
public:
    TerrainsFilterModel(QObject* parent);

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};

class GUIGAMEWRAPPERS_EXPORT MapBanksModel : public AbstractGuiWrapperListModel<GuiMapBank> {
public:
    using Base = AbstractGuiWrapperListModel<GuiMapBank>;
    using AbstractGuiWrapperListModel::AbstractGuiWrapperListModel;

    enum MapItemDataRole
    {
        VaraintNames = LastBaseRole + 1,
    };

    QVariant data(const QModelIndex& index, int role) const override;
};

class UiCommonModel;
class IAppSettings;
class GUIGAMEWRAPPERS_EXPORT LibraryModelsProvider : public QObject {
    Q_OBJECT
public:
    LibraryModelsProvider(const Core::IGameDatabase* gameDatabase,
                          Sound::IMusicBox*          musicBox,
                          const IGraphicsLibrary*    graphicsLibrary,
                          const IAppSettings*        appSettings,
                          QObject*                   parent = nullptr);

    // setModel(QAbstractModel*) need non-const model pointer.
    // that's why all models here are non-const.
    // clang-format off
    ArtifactsModel   * artifacts  () const noexcept { return m_artifacts;}
    UnitsModel       * units      () const noexcept { return m_units;}
    HeroesModel      * heroes     () const noexcept { return m_heroes;}
    SkillsModel      * skills     () const noexcept { return m_skills;}
    SpellsModel      * spells     () const noexcept { return m_spells;}
    FactionsModel    * factions   () const noexcept { return m_factions;}
    TerrainsModel    * terrains   () const noexcept { return m_terrains;}
    MapBanksModel    * mapBanks   () const noexcept { return m_mapBanks;}
    UiCommonModel    * ui         () const noexcept { return m_uiCommon;}
    // clang-format on
    const IAppSettings* appSettings() const noexcept { return m_appSettings; }

private:
    ArtifactsModel* m_artifacts;
    UnitsModel*     m_units;
    HeroesModel*    m_heroes;
    SkillsModel*    m_skills;
    SpellsModel*    m_spells;
    FactionsModel*  m_factions;
    TerrainsModel*  m_terrains;
    MapBanksModel*  m_mapBanks;
    UiCommonModel*  m_uiCommon;

    const IAppSettings* const m_appSettings;
};

}
