/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include <QWidget>

#include <memory>
#include <functional>
#include <array>

class QLabel;

namespace FreeHeroes {
namespace Core {
struct HeroPrimaryParams;
enum class HeroPrimaryParamType;
}

namespace Gui {
class LibraryModelsProvider;
class DarkFrameLabelIcon;
class HoverHelper;
class GUIWIDGETS_EXPORT PrimaryStatsWidget : public QWidget {
    Q_OBJECT
public:
    PrimaryStatsWidget(QWidget* parent = nullptr);
    ~PrimaryStatsWidget();

    void setParams(const Core::HeroPrimaryParams& primary, const LibraryModelsProvider* modelsProvider);

    void setHoverLabel(QLabel* hoverLabel);

private:
    std::array<QLabel*, 4>             m_valueTxt;
    std::array<QLabel*, 4>             m_titleTxt;
    std::array<DarkFrameLabelIcon*, 4> m_iconsLabels;
    QList<Core::HeroPrimaryParamType>  m_params;

    std::unique_ptr<HoverHelper> m_hoverHelper;
};

}
}
