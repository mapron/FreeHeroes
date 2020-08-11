/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureWrappers.hpp"

#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QConcatenateTablesProxyModel>

namespace FreeHeroes::Gui {
class ArtifactsFilterModel;
class HeroBagEditModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    HeroBagEditModel(ArtifactsModel * artifacts, Core::AdventureHeroMutablePtr source, QObject * parent);
    ~HeroBagEditModel();

    void refresh();

    // QAbstractItemModel interface
private:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant & value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;


private:
    ArtifactsModel * m_artifacts = nullptr;
    Core::AdventureHeroMutablePtr m_source = nullptr;
    ArtifactsFilterModel * m_filter = nullptr;

};



class HeroWearingEditModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    HeroWearingEditModel(ArtifactsModel * artifacts, Core::AdventureHeroMutablePtr source, QObject * parent);
    ~HeroWearingEditModel();

    void refresh();

private:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant & value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    ArtifactsModel * m_artifacts = nullptr;
    Core::AdventureHeroMutablePtr m_source = nullptr;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};



class HeroSkillsEditModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    HeroSkillsEditModel(SkillsModel * skillsModel, Core::AdventureHeroMutablePtr source, QObject * parent);
    ~HeroSkillsEditModel();

    void refresh();

private:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant & value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    SkillsModel * m_skills = nullptr;
    Core::AdventureHeroMutablePtr m_source = nullptr;
};

class SpellsFilterModel;
class HeroSpellbookEditModel : public QAbstractListModel
{
    Q_OBJECT
public:
    HeroSpellbookEditModel(SpellsModel * spellsModel, Core::AdventureHeroMutablePtr source, QObject * parent);
    ~HeroSpellbookEditModel();

    void refresh();

private:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant & value, int role) override;

private:
    SpellsModel * m_spellsModel = nullptr;
    Core::AdventureHeroMutablePtr m_source = nullptr;
    SpellsFilterModel * m_filter = nullptr;
};

}
