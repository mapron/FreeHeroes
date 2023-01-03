/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ConverterDialog.hpp"

#include "ui_ConverterDialog.h"

#include "IGameDatabase.hpp"

#include "GameExtract.hpp"

// Gui
#include "FsUtilsQt.hpp"
#include "EnvDetect.hpp"

// Platform
#include "MernelPlatform/ShellUtils.hpp"
#include "MernelPlatform/Logger.hpp"

#include <QStandardPaths>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>

#include <thread>

namespace FreeHeroes {
using namespace Core;
using namespace Gui;
using namespace Mernel;

namespace {

void displayStatus(QLabel* label, bool status)
{
    label->setStyleSheet(QString("QLabel { border-image: url(:/Application/%1.png); }").arg(status ? "check_icon" : "cross_icon"));
}

}

ConverterDialog::ConverterDialog(const Core::IGameDatabaseContainer* databaseContainer, QWidget* parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::ConverterDialog>())
    , m_converterSettings(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/ConverterSettings.ini", QSettings::IniFormat)
    , m_databaseContainer(databaseContainer)
{
    m_ui->setupUi(this);

    QString localData = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    if (localData.endsWith("/") || localData.endsWith("\\"))
        localData = localData.mid(0, localData.size() - 1);
    m_ui->dstPath->setText(localData + "/Resources/Imported");
    m_ui->dstPath->setFrame(false);

    QString path = m_converterSettings.value("srcPath").toString();
    if (path.isEmpty()) {
        path = Gui::stdPath2QString(findHeroes3Installation());
        Logger(Logger::Info) << "Guessing installation directory: " << path.toStdString();
    }
    m_ui->srcPath->setText(path);

    setWindowTitle(tr("Resource converter"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->pushButtonConvert->setFocus();
    m_ui->srcPath->setFocusPolicy(Qt::ClickFocus);

    connect(m_ui->srcPath, &QLineEdit::textChanged, this, &ConverterDialog::pathChanged);
    connect(m_ui->pushButtonBrowse, &QPushButton::clicked, this, &ConverterDialog::browsePath);
    connect(m_ui->pushButtonConvert, &QPushButton::clicked, this, &ConverterDialog::run);
    connect(m_ui->pushButtonShowUnpack, &QPushButton::clicked, this, [this] {
        showFolderInFileManager(Gui::QString2stdPath(m_ui->dstPath->text()));
    });
    connect(this, &ConverterDialog::progressInternal, this, &ConverterDialog::progress, Qt::QueuedConnection);
    connect(this, &ConverterDialog::progress, this, [this](int current, int total) {
        m_ui->progressBar->setValue(current);
        m_ui->progressBar->setMaximum(total);
    });
    connect(
        this, &ConverterDialog::finished, this, [this] {
            m_ui->pushButtonConvert->setEnabled(true);
            m_ui->progressBar->setValue(0);
            m_ui->progressBar->setMaximum(1);
            auto elapsed = QDateTime::currentSecsSinceEpoch() - m_start;
            statusUpdate(tr("Done in %1 seconds.").arg(elapsed));

            if (m_thread.joinable())
                m_thread.join();
        },
        Qt::QueuedConnection);
    connect(m_ui->pushButtonCleanupUnpack, &QPushButton::clicked, this, &ConverterDialog::cleanUnpackDestination);
    pathChanged();

    GameExtract extractor(m_databaseContainer, GameExtract::Settings{ .m_heroesRoot = m_hotaInstallDir });
    auto        res = extractor.probe();

    const bool ffmpegFound = !res.m_ffmpegPath.empty();
    displayStatus(m_ui->labelFfmpegFoundStatus, ffmpegFound);
    if (ffmpegFound) {
        m_ui->labelFFMpegPath->setText(stdPath2QString(res.m_ffmpegPath));
    } else {
        m_ui->labelFfmpegFound->setText(tr("FFmpeg not in the PATH, no effect and video will be converted."));
    }
}

ConverterDialog::~ConverterDialog()
{
    if (m_thread.joinable())
        m_thread.join();
}
void ConverterDialog::browsePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open HotA installation root"));
    if (dir.isEmpty())
        return;
    m_ui->srcPath->setText(dir);
}

void ConverterDialog::pathChanged()
{
    m_hotaInstallDir = QString2stdPath(m_ui->srcPath->text());

    GameExtract extractor(m_databaseContainer, GameExtract::Settings{ .m_heroesRoot = m_hotaInstallDir });
    auto        res = extractor.probe();
    displayStatus(m_ui->labelSodFoundStatus, res.m_hasSod);
    displayStatus(m_ui->labelHotaFoundStatus, res.m_hasHota);
    displayStatus(m_ui->labelHdFoundStatus, res.m_hasHD);

    m_ui->pushButtonConvert->setEnabled(res.m_hasSod);
    if (res.m_hasSod) {
        m_converterSettings.setValue("srcPath", m_ui->srcPath->text());
    }
}

void ConverterDialog::run()
{
    m_start = QDateTime::currentSecsSinceEpoch();
    m_ui->pushButtonConvert->setEnabled(false);
    m_thread = std::thread([this] {
        const std_path baseExtract = QString2stdPath(m_ui->dstPath->text());
        const std_path archives    = baseExtract.parent_path() / "Archives";

        GameExtract extractor(m_databaseContainer, GameExtract::Settings{ .m_heroesRoot = m_hotaInstallDir, .m_archiveExtractRoot = archives, .m_mainExtractRoot = baseExtract });
        auto        res = extractor.probe();
        extractor.setProgressCallback([this](int current, int total) { emit progressInternal(current, total); });
        extractor.run(res);

        emit finished();
    });
}

void ConverterDialog::cleanUnpackDestination()
{
    if (QMessageBox::warning(this, tr("Removing folder contents"), tr("Are you sure?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    m_ui->pushButtonCleanupUnpack->setEnabled(false);

    const std_path       baseExtract = QString2stdPath(m_ui->dstPath->text());
    std::deque<std_path> forDelete;
    for (auto&& it : std_fs::recursive_directory_iterator(baseExtract)) {
        if (it.is_regular_file())
            forDelete.push_back(it.path());
    }
    statusUpdate(tr("Files are removing..."));
    auto lastUpd = std::chrono::milliseconds{ 0 };
    int  done    = 0;
    m_ui->progressBar->setMaximum(forDelete.size());
    m_ui->progressBar->setValue(0);
    for (const auto& path : forDelete) {
        std::error_code ec;
        std_fs::remove(path, ec);
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
        done++;
        if (now > lastUpd) {
            m_ui->progressBar->setValue(done);
            lastUpd = now + std::chrono::milliseconds{ 100 };
        }
    }

    m_ui->pushButtonCleanupUnpack->setEnabled(true);
    m_ui->progressBar->setValue(0);
    m_ui->progressBar->setMaximum(1);
    statusUpdate(tr("Cleanup done."));
}

void ConverterDialog::statusUpdate(QString txt)
{
    Logger(Logger::Info) << txt.toStdString();
    m_ui->labelStatus->setText(txt);
    m_ui->labelStatus->updateGeometry();
    m_ui->labelStatus->repaint();
}

}
