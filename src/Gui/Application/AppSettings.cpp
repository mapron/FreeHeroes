/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AppSettings.hpp"

#include "SettingsWidget.hpp"

#include <QSettings>
#include <QStandardPaths>

namespace FreeHeroes::Gui {

class AppSettings::QVariantRef {
    enum class Type
    {
        Bool,
        Int,
        String
    };
    QString  name;
    Type     type      = Type::Bool;
    bool*    refBool   = nullptr;
    int*     refInt    = nullptr;
    QString* refString = nullptr;

public:
    QVariantRef(QString name, bool& value)
        : name(name)
        , type(Type::Bool)
        , refBool(&value)
    {}
    QVariantRef(QString name, int& value)
        : name(name)
        , type(Type::Int)
        , refInt(&value)
    {}
    QVariantRef(QString name, QString& value)
        : name(name)
        , type(Type::String)
        , refString(&value)
    {}

    QVariant get() const
    {
        if (type == Type::Bool)
            return QVariant(*refBool);
        if (type == Type::String)
            return QVariant(*refString);
        if (type == Type::Int)
            return QVariant(*refInt);
        return {};
    }
    void set(const QVariant& data)
    {
        if (type == Type::Bool)
            *refBool = data.toBool();
        if (type == Type::String)
            *refString = data.toString();
        if (type == Type::Int)
            *refInt = data.toInt();
    }

    void write(QSettings& settings) const
    {
        settings.setValue(name, get());
    }
    void read(const QSettings& settings)
    {
        set(settings.value(name, get()));
    }
};

class AppSettings::QVariantRefList {
public:
    QString                         key;
    QList<AppSettings::QVariantRef> children;

    void write(QSettings& settings) const
    {
        settings.beginGroup(key);
        for (auto& child : children)
            child.write(settings);
        settings.endGroup();
    }
    void read(QSettings& settings)
    {
        settings.beginGroup(key);
        for (auto& child : children)
            child.read(settings);
        settings.endGroup();
    }
};

AppSettings::AppSettings(QString filename)
    : QObject(nullptr)
    , m_settings(filename, QSettings::IniFormat)
{
    // clang-format off
    m_allWrappers.push_back({
        "sound", {
            {"musicVolumePercent"  , m_all.sound.musicVolumePercent},
            {"effectsVolumePercent", m_all.sound.effectsVolumePercent},
        }
    });
    m_allWrappers.push_back({
        "battle", {
            {"shiftSpeedupPercent", m_all.battle.shiftSpeedPercent},
            {"walkTimePercent"    , m_all.battle.walkTimePercent},
            {"otherTimePercent"   , m_all.battle.otherTimePercent},
            {"displayGrid"        , m_all.battle.displayGrid},
            {"displayPath"        , m_all.battle.displayPath},
            {"logMoves"           , m_all.battle.logMoves},
            {"retaliationHint"    , m_all.battle.retaliationHint},
            {"massDamageHint"     , m_all.battle.massDamageHint},
            {"counterDamageHint"  , m_all.battle.counterDamageHint},
        }
    });
    m_allWrappers.push_back({
        "global", {
            {"logLevel", m_all.global.logLevel},
            {"localeId", m_all.global.localeId},
            {"databaseId", m_all.global.databaseId},
        }
    });
    m_allWrappers.push_back({
        "ui", {
            {"displayAbsMoraleLuck", m_all.ui.displayAbsMoraleLuck},
            {"clampAbsMoraleLuck"  , m_all.ui.clampAbsMoraleLuck},
        }
    });
    // clang-format on
}

AppSettings::~AppSettings() = default;

void AppSettings::load()
{
    for (auto& wrap : m_allWrappers)
        wrap.read(m_settings);
}

void AppSettings::save()
{
    for (auto& wrap : m_allWrappers)
        wrap.write(m_settings);
}

void AppSettings::showSettingsEditor(QWidget* parent)
{
    Gui::SettingsWidget w(m_settings, m_all, parent);
    if (w.exec() != QDialog::Accepted)
        return;
    save();

    emit setMusicVolume(m_all.sound.musicVolumePercent);
    emit setEffectsVolume(m_all.sound.effectsVolumePercent);
}

std::unique_ptr<QSettings> getUiDefaultSettings()
{
    QString filename = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/AppSettings.ini";
    return std::make_unique<QSettings>(filename, QSettings::IniFormat);
}

}
