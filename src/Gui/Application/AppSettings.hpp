/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IAppSettings.hpp"

#include <QString>
#include <QObject>
#include <QSettings>

#include <memory>
#include <vector>


namespace FreeHeroes::Gui {


class AppSettings : public QObject, public IAppSettings {

    Q_OBJECT
public:
    AppSettings(QString filename);
    ~AppSettings();

signals:
    void setMusicVolume(int percent);
    void setEffectsVolume(int percent);

public:

    void load();
    void save();

    const Sound & sound() const  noexcept override{ return m_all.sound; }

    const Battle & battle() const noexcept override{ return m_all.battle;}

    const AppGlobal & global() const noexcept override{ return m_all.global;}
    const UI & ui() const noexcept override { return m_all.ui;}

    void showSettingsEditor(QWidget * parent) override;

    QSettings & uiSettings() override { return m_settings; }

    AppGlobal & globalMutable() { return m_all.global; }

private:
    class QVariantRef;
    class QVariantRefList;
    std::vector<QVariantRefList> m_allWrappers;

    AllSettings m_all;
    QString m_filename;
    QSettings m_settings;
};


}
