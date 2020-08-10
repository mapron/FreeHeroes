/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SettingsWidget.hpp"
#include "ui_SettingsWidget.h"


#include <QSettings>
#include <QComboBox>

#include <functional>


namespace FreeHeroes::Gui {


class SettingsWidget::WidgetWrapRef {
    enum class Type { Bool, Int, StringCombo };

    Type type = Type::Bool;
    bool * refBool = nullptr;
    int * refInt = nullptr;
    QString * refStr = nullptr;
    QCheckBox * refCb = nullptr;
    QSpinBox * refSb = nullptr;
    QComboBox * refCombo = nullptr;
    QStringList items;
public:
    WidgetWrapRef(bool & value, QCheckBox * cb) : type(Type::Bool), refBool(&value), refCb(cb){}
    WidgetWrapRef(int & value, QSpinBox * sb) : type(Type::Int), refInt(&value), refSb(sb){}
    WidgetWrapRef(QString & value, QStringList items, QComboBox * c) : type(Type::StringCombo), refStr(&value), refCombo(c), items(items){}

    void read() {
        if (type == Type::Bool)           refCb->setChecked(*refBool);
        if (type == Type::Int)            refSb->setValue(*refInt);
        if (type == Type::StringCombo)   {
            refCombo->addItems(items);
            refCombo->setCurrentIndex(items.indexOf(*refStr));
        }
    }
    void write() {
        if (type == Type::Bool)         *refBool   = refCb->isChecked();
        if (type == Type::Int)          *refInt    = refSb->value();
        if (type == Type::StringCombo)  *refStr    = refCombo->currentText();
    }
};

SettingsWidget::SettingsWidget(QSettings & uiSettings, IAppSettings::AllSettings & settings, QWidget* parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::SettingsWidget>())
{
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    m_ui->setupUi(this);

    m_allRefs = {
        {settings.sound.musicVolumePercent   , m_ui->spinBoxMusicVolume},
        {settings.sound.effectsVolumePercent , m_ui->spinBoxEffectsVolume},

        { settings.battle.shiftSpeedPercent, m_ui->spinBoxShiftSpeed},
        { settings.battle.walkTimePercent  , m_ui->spinBoxBattleMovementTime},
        { settings.battle.otherTimePercent, m_ui->spinBoxBattleGeneralTime},
        { settings.battle.displayGrid     , m_ui->checkBoxDisplayGrid},
        { settings.battle.displayPath     , m_ui->checkBoxDisplayPath},
        { settings.battle.logMoves        , m_ui->checkBoxLogMoves},
        { settings.battle.retaliationHint , m_ui->checkBoxRetaliationHint},

        { settings.global.logLevel , m_ui->spinBoxLogLevel},
        { settings.global.localeId   , settings.global.localeItems   , m_ui->comboBoxLocale},
        { settings.global.databaseId , settings.global.databaseItems , m_ui->comboBoxDatabase},

        { settings.ui.displayAbsMoraleLuck , m_ui->checkBoxAbsRng},
        { settings.ui.clampAbsMoraleLuck , m_ui->checkBoxClampRng},
    };


    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsWidget::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsWidget::reject);

    auto setupSliderSpin = [this](QSlider * slider, QSpinBox * spinbox, int min, int max) {
        static const int step = 5;
        slider->setMinimum(min / step);
        spinbox->setMinimum(min);
        slider->setMaximum(max / step);
        spinbox->setMaximum(max);
        this->connect(slider, &QSlider::sliderMoved, spinbox, [spinbox](int value){ spinbox->setValue(value * step); });

        this->connect(spinbox, qOverload<int>(&QSpinBox::valueChanged), slider, [slider](int value){ slider->setValue(value / step); });
    };
    setupSliderSpin(m_ui->horizontalSliderMusicVolume, m_ui->spinBoxMusicVolume, 0, 100);
    setupSliderSpin(m_ui->horizontalSliderEffectsVolume, m_ui->spinBoxEffectsVolume, 0, 100);
    setupSliderSpin(m_ui->horizontalSliderShiftSpeed, m_ui->spinBoxShiftSpeed, 0, 80);
    setupSliderSpin(m_ui->horizontalSliderBattleMovementTime, m_ui->spinBoxBattleMovementTime, 0, 150);
    setupSliderSpin(m_ui->horizontalSliderBattleGeneralTime, m_ui->spinBoxBattleGeneralTime, 0, 150);


    uiSettings.beginGroup("SettingsWidget");
    const int tabIndex = uiSettings.value("tabIndex").toInt();
    uiSettings.endGroup();
    m_ui->tabWidget->setCurrentIndex(tabIndex);

    connect(this, &QDialog::finished, this, [&uiSettings, this]{

        const int tabIndex = m_ui->tabWidget->currentIndex();

        uiSettings.beginGroup("SettingsWidget");
        uiSettings.setValue("tabIndex", tabIndex);
        uiSettings.endGroup();
    });
    for (auto & wrap : m_allRefs) {
        wrap.read();
    }
}


SettingsWidget::~SettingsWidget() = default;

void SettingsWidget::accept()
{
    QDialog::accept();
    for (auto & wrap : m_allRefs) {
        wrap.write();
    }
}

void SettingsWidget::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

}
