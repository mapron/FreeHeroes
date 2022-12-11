/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ConverterDialog.hpp"

#include "ui_ConverterDialog.h"

#include "ArchiveParser.hpp"
#include "LocalizationConverter.hpp"
#include "ThreadPoolExecutor.hpp"
#include "KnownResources.hpp"
#include "ResourcePostprocess.hpp"
#include "MediaConverter.hpp"
#include "GameConstants.hpp"
#include "IGameDatabase.hpp"

// Gui
#include "FsUtilsQt.hpp"
#include "ResourceLibrary.hpp"

// Platform
#include "ShellUtils.hpp"
#include "Logger.hpp"

#include <QStandardPaths>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>

#include <thread>

namespace FreeHeroes::Conversion {
using namespace Core;
using namespace Gui;

namespace {
QString guessInstallationPath()
{
#ifdef _WIN32
    QSettings reg(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HotA + HD_is1",
        QSettings::Registry32Format);
    return reg.value("InstallLocation").toString();
#else
    return {};
#endif
}
}

ConverterDialog::ConverterDialog(const Core::IGameDatabaseContainer* databaseContainer, QWidget* parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::ConverterDialog>())
    , m_converterSettings(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/ConverterSettings.ini", QSettings::IniFormat)
    , m_databaseContainer(databaseContainer)
{
    m_ui->setupUi(this);
    for (auto* cb : { m_ui->checkBoxHdFound, m_ui->checkBoxSodFound, m_ui->checkBoxHotaFound, m_ui->checkBoxFfmpegFound }) {
        cb->setAttribute(Qt::WA_TransparentForMouseEvents);
        cb->setFocusPolicy(Qt::NoFocus);
    }

    QString localData = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    if (localData.endsWith("/") || localData.endsWith("\\"))
        localData = localData.mid(0, localData.size() - 1);
    m_ui->dstPath->setText(localData + "/Resources/Imported");
    m_ui->dstPath->setFrame(false);

    QString path = m_converterSettings.value("srcPath").toString();
    if (path.isEmpty()) {
        path = guessInstallationPath();
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
    connect(m_ui->pushButtonCleanupUnpack, &QPushButton::clicked, this, &ConverterDialog::cleanUnpackDestination);
    pathChanged();

    MediaConverter conv;
    const bool     ffmpegFound = conv.ffmpegFound();
    if (ffmpegFound) {
        m_ui->labelFFMpegPath->setText(conv.ffmpegBinary());
        m_ui->checkBoxFfmpegFound->setChecked(true);
    } else {
        m_ui->checkBoxFfmpegFound->setText(tr("FFmpeg not in the PATH, no effect and video will be converted."));
    }
}

ConverterDialog::~ConverterDialog() = default;

void ConverterDialog::browsePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open HotA installation root"));
    if (dir.isEmpty())
        return;
    m_ui->srcPath->setText(dir);
}

void ConverterDialog::pathChanged()
{
    m_hotaInstallDir                  = QString2stdPath(m_ui->srcPath->text());
    const std_path hotaInstallDirData = m_hotaInstallDir / "Data";
    const bool     sodExists          = std_fs::exists(hotaInstallDirData / "H3sprite.lod");
    const bool     hotaExists         = std_fs::exists(hotaInstallDirData / "HotA.lod");
    const bool     hdExists           = std_fs::exists(m_hotaInstallDir / "_HD3_Data" / "Common");
    m_ui->checkBoxSodFound->setChecked(sodExists);
    m_ui->checkBoxHotaFound->setChecked(hotaExists);
    m_ui->checkBoxHdFound->setChecked(hdExists);
    m_ui->pushButtonConvert->setEnabled(sodExists);
    if (sodExists) {
        m_converterSettings.setValue("srcPath", m_ui->srcPath->text());
    }
}

void ConverterDialog::run()
{
    m_ui->pushButtonConvert->setEnabled(false);
    auto start = QDateTime::currentSecsSinceEpoch();

    const bool sodExists       = m_ui->checkBoxSodFound->isChecked();
    const bool hotaExists      = m_ui->checkBoxHotaFound->isChecked();
    const bool hdExists        = m_ui->checkBoxHdFound->isChecked();
    const bool ffmpegAvailable = m_ui->checkBoxFfmpegFound->isChecked();

    QStringList    args;
    const std_path hotaInstallDirData = m_hotaInstallDir / "Data";
    const std_path baseExtract        = QString2stdPath(m_ui->dstPath->text());

    const bool overrideExisting = args.contains("--override-existing");
    const bool keepTmp          = args.contains("--keep-tmp");

    QSet<Core::ResourceMedia::Type> requiredTypes;
    if (args.contains("--audio"))
        requiredTypes << Core::ResourceMedia::Type::Sound << Core::ResourceMedia::Type::Music;
    if (args.contains("--video"))
        requiredTypes << Core::ResourceMedia::Type::Video;
    if (args.contains("--sprites"))
        requiredTypes << Core::ResourceMedia::Type::Sprite;
    if (args.contains("--other"))
        requiredTypes << Core::ResourceMedia::Type::Other;
    if (requiredTypes.isEmpty()) {
        requiredTypes << Core::ResourceMedia::Type::Sound
                      << Core::ResourceMedia::Type::Music
                      << Core::ResourceMedia::Type::Video
                      << Core::ResourceMedia::Type::Sprite
                      << Core::ResourceMedia::Type::Other;
    }

    std::unique_ptr<ResourceLibrary> sodLibrary;
    std::unique_ptr<ResourceLibrary> hotaLibrary;
    std::unique_ptr<ResourceLibrary> hdLibrary;
    ArchiveParser::ExtractionList    extractionList;

    const std_path baseExtractSOD  = baseExtract / "SOD";
    const std_path baseExtractHOTA = baseExtract / "HOTA";
    const std_path baseExtractHD   = baseExtract / "HD";
    if (sodExists) {
        sodLibrary = std::make_unique<ResourceLibrary>("sod");
        sodLibrary->setIndexFolder(baseExtractSOD);
        sodLibrary->loadIndex();
        if (ffmpegAvailable) {
            extractionList.push_back({ ArchiveParser::TaskType::SND,
                                       hotaInstallDirData,
                                       "Heroes3.snd",
                                       baseExtractSOD,
                                       sodLibrary.get() });
            extractionList.push_back({ ArchiveParser::TaskType::VID,
                                       hotaInstallDirData,
                                       "VIDEO.VID",
                                       baseExtractSOD,
                                       sodLibrary.get() });
        }

        extractionList.push_back({ ArchiveParser::TaskType::LOD,
                                   hotaInstallDirData,
                                   "H3sprite.lod",
                                   baseExtractSOD,
                                   sodLibrary.get() });
        extractionList.push_back({ ArchiveParser::TaskType::LOD,
                                   hotaInstallDirData,
                                   "H3bitmap.lod",
                                   baseExtractSOD,
                                   sodLibrary.get() });

        // workaround: sometimes folder called 'Mp3' in the distribution.
        auto mp3path = m_hotaInstallDir / "MP3";
        if (!std_fs::exists(mp3path))
            mp3path = m_hotaInstallDir / "Mp3";
        extractionList.push_back({ ArchiveParser::TaskType::MusicCopy,
                                   mp3path,
                                   "",
                                   baseExtractSOD,
                                   sodLibrary.get() });
    }
    if (hotaExists) {
        hotaLibrary = std::make_unique<ResourceLibrary>("hota");
        hotaLibrary->addDep("sod");
        hotaLibrary->setIndexFolder(baseExtractHOTA);
        hotaLibrary->loadIndex();
        if (ffmpegAvailable) {
            extractionList.push_back({ ArchiveParser::TaskType::SND,
                                       hotaInstallDirData,
                                       "HotA.snd",
                                       baseExtractHOTA,
                                       hotaLibrary.get() });
            extractionList.push_back({ ArchiveParser::TaskType::VID,
                                       hotaInstallDirData,
                                       "HotA.vid",
                                       baseExtractHOTA,
                                       hotaLibrary.get() });
        }

        extractionList.push_back({ ArchiveParser::TaskType::LOD,
                                   hotaInstallDirData,
                                   "HotA.lod",
                                   baseExtractHOTA,
                                   hotaLibrary.get() });

        extractionList.push_back({ ArchiveParser::TaskType::LOD,
                                   hotaInstallDirData,
                                   "HotA_lng.lod",
                                   baseExtractHOTA,
                                   hotaLibrary.get() });

        extractionList.push_back({ ArchiveParser::TaskType::HDAT,
                                   m_hotaInstallDir,
                                   "HotA.dat",
                                   baseExtractHOTA,
                                   hotaLibrary.get() });
    }
    if (hdExists) {
        hdLibrary = std::make_unique<ResourceLibrary>("hdmod");
        hdLibrary->addDep("sod");
        hdLibrary->setIndexFolder(baseExtractHD);
        hdLibrary->loadIndex();

        extractionList.push_back({ ArchiveParser::TaskType::DefCopy,
                                   m_hotaInstallDir / "_HD3_Data" / "Common",
                                   "",
                                   baseExtractHD,
                                   hdLibrary.get() });
        extractionList.push_back({ ArchiveParser::TaskType::DefCopy,
                                   m_hotaInstallDir / "_HD3_Data" / "Common" / "Fix.Cosmetic",
                                   "",
                                   baseExtractHD,
                                   hdLibrary.get() });
    }
    int total = 0;
    statusUpdate(tr("DB init..."));

    auto* hotaDb = m_databaseContainer->getDatabase(GameVersion::HOTA);
    auto* sodDb  = m_databaseContainer->getDatabase(GameVersion::SOD);

    statusUpdate(tr("Extracting files..."));

    KnownResources knownResources(QString2stdPath(QApplication::applicationDirPath()) / "gameResources" / "knownResources.txt");
    auto           lastUpd     = std::chrono::milliseconds{ 0 };
    int            doneExtract = 0;
    ArchiveParser  parser(knownResources,
                         requiredTypes,
                         overrideExisting,
                         keepTmp,
                         [this, &lastUpd, &doneExtract] {
                             auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
                             doneExtract++;
                             if (now > lastUpd) {
                                 m_ui->progressBar->setValue(doneExtract);
                                 lastUpd = now + std::chrono::milliseconds{ 100 };
                             }
                         });
    total = parser.estimateExtractCount(extractionList);

    m_ui->progressBar->setMaximum(total);
    m_ui->progressBar->setValue(0);

    statusUpdate(tr("Preparing conversion list..."));

    ThreadPoolExecutor              executor;
    ArchiveParser::CallbackInserter inserter([&executor](auto task) {
        executor.add(task);
    });
    parser.prepareExtractTasks(extractionList, inserter);

    Q_ASSERT(doneExtract == total);

    statusUpdate(tr("Conversion of media..."));

    m_ui->progressBar->setValue(0);
    m_ui->progressBar->setMaximum(executor.getQueueSize());

    QEventLoop loop;
    connect(&executor, &ThreadPoolExecutor::finished, &loop, &QEventLoop::quit);
    connect(&executor, &ThreadPoolExecutor::progress, m_ui->progressBar, &QProgressBar::setValue);
    executor.start(std::chrono::milliseconds{ 100 });

    Logger(Logger::Info) << "Event loop start";
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    Logger(Logger::Info) << "Event loop end";

    statusUpdate(tr("Translation generation..."));

    if (sodExists) {
        Logger(Logger::Info) << "SoD";
        LocalizationConverter converter(*sodLibrary, baseExtractSOD, hotaDb, sodDb);
        converter.extractSOD("txt");

        sodLibrary->saveIndex();
    }

    if (hotaExists) {
        Logger(Logger::Info) << "HotA";
        LocalizationConverter converter(*hotaLibrary, baseExtractHOTA, hotaDb, sodDb);
        converter.extractSOD("txt");
        converter.extractHOTA("json");

        Logger(Logger::Info) << "HotA PP";
        ResourcePostprocess pp;
        pp.concatSprites(*hotaLibrary, { "cmbkhlmt0", "cmbkhlmt1", "cmbkhlmt2", "cmbkhlmt3", "cmbkhlmt4" }, "cmbkhlmt", true);
        hotaLibrary->saveIndex();
    }
    if (hdExists) {
        hdLibrary->saveIndex();
    }
    m_ui->progressBar->setValue(0);
    m_ui->progressBar->setMaximum(1);
    auto elapsed = QDateTime::currentSecsSinceEpoch() - start;
    statusUpdate(tr("Done in %1 seconds.").arg(elapsed));

    m_ui->pushButtonConvert->setEnabled(true);
}

void ConverterDialog::cleanUnpackDestination()
{
    if (QMessageBox::warning(this, tr("Removing folder contents"), tr("Are you sure?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    m_ui->pushButtonCleanupUnpack->setEnabled(false);

    const std_path       baseExtract = QString2stdPath(m_ui->dstPath->text());
    std::deque<std_path> forDelete;
    for (auto it : std_fs::recursive_directory_iterator(baseExtract)) {
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
