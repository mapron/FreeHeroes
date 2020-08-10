/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ArtifactSelectorWidget.hpp"

#include "LibraryEditorModels.hpp"
#include "LibraryWrappersMetatype.hpp"

#include "Profiler.hpp"

namespace FreeHeroes::Gui {

using namespace Core;

ArtifactSelectorWidget::ArtifactSelectorWidget(QWidget* parent)
    : ResizeableComboBox(parent)
{

    m_filterModel = new ArtifactsFilterModel(this);

    m_finalModel = new ArtifactsComboModel(m_filterModel, this);

    this->setIconSize({22,22});
}

void ArtifactSelectorWidget::setSlotFilter(int filter)
{
    m_filterModel->setFilterIndex(filter);
}

Core::LibraryArtifactConstPtr ArtifactSelectorWidget::getArtifact() const
{
    auto art = this->currentData(ArtifactsModel::SourceObject).value<Core::LibraryArtifactConstPtr>();
    return art;
}

void ArtifactSelectorWidget::setArtifact(Core::LibraryArtifactConstPtr params)
{
    int currentIndex = 0;
    for (int i = 0, cnt = this->count(); i < cnt; ++i) {

        if (params && this->itemData(i, ArtifactsModel::SourceObject).value<LibraryArtifactConstPtr>() == params) {
            currentIndex = i;
            break;
        }
    }
    this->setCurrentIndex(currentIndex);
}

void ArtifactSelectorWidget::setArtifactsModel(QAbstractItemModel* rootArtifactsModel)
{
    m_filterModel->setSourceModel(rootArtifactsModel);
    this->setModel(m_finalModel);
}

ArtifactSelectorWidget::~ArtifactSelectorWidget() = default;

QWidget* ArtifactSelectDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    const int slotFilter = index.data(Qt::UserRole).toInt();
    auto * cb = new ArtifactSelectorWidget(parent);
    cb->setArtifactsModel(m_rootArtifactsModel);
    cb->setSlotFilter(slotFilter);

    connect(cb,  QOverload<int>::of(&QComboBox::activated),
            this, &ArtifactSelectDelegate::commitAndCloseEditor);

    return cb;
}

void ArtifactSelectDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto art = index.data(Qt::UserRole + 1).value<LibraryArtifactConstPtr>();
    auto cb = qobject_cast<ArtifactSelectorWidget*>(editor);
    cb->setArtifact(art);
    cb->showPopup();
}

void ArtifactSelectDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto cb = qobject_cast<ArtifactSelectorWidget*>(editor);
    auto art = cb->getArtifact();
    model->setData(index, QVariant::fromValue(art), Qt::UserRole + 1);
}

void ArtifactSelectDelegate::commitAndCloseEditor(){
    ArtifactSelectorWidget *editor = qobject_cast<ArtifactSelectorWidget *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

}
