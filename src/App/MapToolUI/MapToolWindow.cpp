/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapToolWindow.hpp"

#include "ui_MapToolWindow.h"

#include "TemplateSettingsWindow.hpp"

#include "MernelPlatform/AppLocations.hpp"

#include "EnvDetect.hpp"

#include <QProcess>
#include <QRandomGenerator64>
#include <QSettings>

namespace FreeHeroes {

namespace {
#ifndef NDEBUG
const QString g_mapEditor = "MapEditorD.exe";
#else
const QString g_mapEditor = "MapEditor.exe";
#endif

const QString g_reg = "HKEY_CURRENT_USER\\SOFTWARE\\FreeHeroes";

}
MapToolWindow::MapToolWindow()
    : m_ui(std::make_unique<Ui::MapToolWindow>())
{
    m_ui->setupUi(this);
    connect(m_ui->pushButtonEditMap, &QAbstractButton::clicked, this, [] {
        QProcess::startDetached(g_mapEditor, {});
    });
    connect(m_ui->pushButtonEditSettings, &QAbstractButton::clicked, this, [this] {
        TemplateSettingsWindow w(this);
        w.exec();
    });

    Mernel::AppLocations loc("FreeHeroes");

    QSettings settings(g_reg, QSettings::NativeFormat);
    m_ui->templatePath->setText(settings.value("templatePath").toString());
    m_ui->templateSettingsPath->setText(settings.value("templateSettingsPath").toString());
    m_ui->fhMapPath->setText(settings.value("fhMapPath").toString());
    m_ui->h3mMapPath->setText(settings.value("h3mMapPath").toString());
    m_ui->lineEditSeed->setText(settings.value("seed").toString());

    Mernel::std_path hotaPath = findHeroes3Installation();

    if (m_ui->templateSettingsPath->text().isEmpty())
        m_ui->templateSettingsPath->setText(QString::fromStdString(Mernel::path2string(loc.getAppdataDir() / "templateSettings.json")));

    if (m_ui->h3mMapPath->text().isEmpty() && !hotaPath.empty()) {
        auto out = hotaPath / "Maps" / "FreeHeroes_output.h3m";
        m_ui->h3mMapPath->setText(QString::fromStdString(Mernel::path2string(out)));
    }
    if (m_ui->fhMapPath->text().isEmpty() && !hotaPath.empty()) {
        auto out = hotaPath / "Maps" / "FreeHeroes_output.json";
        m_ui->fhMapPath->setText(QString::fromStdString(Mernel::path2string(out)));
    }
    if (m_ui->templatePath->text().isEmpty()) {
        auto tpl = loc.getBinDir() / "gameResources" / "templates" / "jebus_balanced.json";
        m_ui->templatePath->setText(QString::fromStdString(Mernel::path2string(tpl)));
    }

    connect(m_ui->pushButtonNewSeed, &QAbstractButton::clicked, this, [this] {
        auto value = QRandomGenerator::global()->generate64();

        m_ui->lineEditSeed->setText(QString::number(value));
    });
}

MapToolWindow::~MapToolWindow()
{
    QSettings settings(g_reg, QSettings::NativeFormat);
    settings.setValue("templatePath", m_ui->templatePath->text());
    settings.setValue("templateSettingsPath", m_ui->templateSettingsPath->text());
    settings.setValue("fhMapPath", m_ui->fhMapPath->text());
    settings.setValue("h3mMapPath", m_ui->h3mMapPath->text());
    settings.setValue("seed", m_ui->lineEditSeed->text());
}

}
