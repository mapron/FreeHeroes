/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CustomFrames.hpp"

#include <QStyledItemDelegate>

#include <memory>

namespace FreeHeroes::Core {
struct LibraryArtifact;
using LibraryArtifactConstPtr = const LibraryArtifact*;

}

namespace FreeHeroes::Gui {
class ArtifactsFilterModel;
class ArtifactsComboModel;
class ArtifactSelectorWidget : public ResizeableComboBox {
    Q_OBJECT
public:
    ArtifactSelectorWidget(QWidget* parent = nullptr);
    ~ArtifactSelectorWidget();

    Core::LibraryArtifactConstPtr getArtifact() const;
    void                          setArtifact(Core::LibraryArtifactConstPtr artifact);

    void setArtifactsModel(QAbstractItemModel* rootArtifactsModel);

    void setSlotFilter(int filter);

private:
    ArtifactsFilterModel* m_filterModel = nullptr;
    ArtifactsComboModel*  m_finalModel  = nullptr;
};

class ArtifactSelectDelegate : public QStyledItemDelegate {
    QAbstractItemModel* m_rootArtifactsModel = nullptr;

public:
    ArtifactSelectDelegate(QAbstractItemModel* rootArtifactsModel, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_rootArtifactsModel(rootArtifactsModel)
    {}

    // QAbstractItemDelegate interface
public:
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    void commitAndCloseEditor();
};

}
