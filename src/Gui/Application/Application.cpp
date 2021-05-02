/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Application.hpp"
#include "GoldenStyle.hpp"

// Gui
#include "GraphicsLibrary.hpp"
#include "LocalizationManager.hpp"
#include "CursorLibrary.hpp"
#include "Profiler.hpp"
#include "AppSettings.hpp"
#include "LibraryModels.hpp"

// Core
#include "FsUtilsQt.hpp"
#include "ResourceLibrary.hpp"
#include "GameDatabase.hpp"
#include "RandomGenerator.hpp"
#include "Logger_details.hpp"

// Sound
#include "MusicBox.hpp"

#include <QApplication>
#include <QStandardPaths>
#include <QSettings>
#include <QResource>
#include <QDebug>

// uncomment to show registered resources in Qt ":/.../" paths.
//#define SHOW_RESOURCES_DEBUG
#ifdef SHOW_RESOURCES_DEBUG
#include <QDirIterator>
#endif

namespace FreeHeroes::Gui {

using namespace Core;

namespace {
constexpr const char* FHFolderName = "FreeHeroes";

void loggerQtOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray  localMsg = msg.toLocal8Bit();
    const char* file     = context.file ? context.file : "";
    //const char *function = context.function ? context.function : "";
    Logger::LogLevel level = Logger::Debug;
    switch (type) {
        case QtDebugMsg:
            level = Logger::Debug;
            break;
        case QtInfoMsg:
            level = Logger::Info;
            break;
        case QtWarningMsg:
            level = Logger::Warning;
            break;
        case QtCriticalMsg:
            level = Logger::Err;
            break;
        case QtFatalMsg:
            level = Logger::Emerg;
            break;
    }
    Logger(level) << "QT:" << localMsg.constData() << "(" << file << ":" << context.line << ")";
}

}

struct Application::Impl {
    std::unique_ptr<LocalizationManager>      locEn;
    std::unique_ptr<LocalizationManager>      locMain;
    std::vector<std::unique_ptr<QTranslator>> translators;
    std_path                                  localDataRoot;
    std_path                                  appBinRoot;
    QString                                   appConfigIniPath;
    std::unique_ptr<AppSettings>              appConfig;

    void addTranslator(const QString& qmPath)
    {
        auto tr = std::make_unique<QTranslator>();
        tr->load(qmPath);
        QApplication::installTranslator(tr.get());
        translators.emplace_back(std::move(tr));
    }
};

Application::Application()
    : m_impl(new Impl)
{
    QApplication::setApplicationName(QString::fromUtf8(FHFolderName));
    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    m_impl->localDataRoot      = QString2stdPath(dataLocation);
    m_impl->appConfigIniPath   = dataLocation + "/AppConfig.ini";
    m_impl->appConfig          = std::make_unique<Gui::AppSettings>(m_impl->appConfigIniPath);
    if (!std_fs::exists(m_impl->localDataRoot))
        std_fs::create_directories(m_impl->localDataRoot);

    m_impl->appConfig->load();
    QString currentLocale = m_impl->appConfig->global().localeId;
    if (currentLocale.isEmpty())
        currentLocale = QLocale::system().name();
    m_impl->appConfig->globalMutable().localeId = currentLocale;
    // to make user happy just edit values, not to guess keys. Do it earlier in case of crash later.
    m_impl->appConfig->save();

    Logger::SetLoggerBackend(std::make_unique<LoggerBackendFiles>(
        m_impl->appConfig->global().logLevel,
        true,  /*duplicateInStderr*/
        true,  /*outputLoglevel   */
        false, /*outputTimestamp  */
        true,  /*outputTimeoffsets*/
        10,
        50000,
        m_impl->localDataRoot / "Logs"));
    qInstallMessageHandler(loggerQtOutput);
    Logger(Logger::Notice) << "Application started. Log level is " << m_impl->appConfig->global().logLevel;

    qWarning() << "checking Qt logs";
}

Application::~Application()
{
    m_impl->appConfig->save();
}

void Application::load(const std::string& moduleName, std::set<Option> options)
{
    Logger(Logger::Info) << "Application::load - start";
    const QString appBin = QApplication::applicationDirPath();
    m_impl->appBinRoot   = QString2stdPath(appBin);
    ProfilerScope scopeAll("Application::load");
    if (options.contains(Option::QtTranslations)) {
        ProfilerScope scope("Qt init resource");
        QStringList   qrc{ "Application", "Battle", "Translations" };
        for (QString qrcName : qrc) {
            [[maybe_unused]] bool registered = QResource::registerResource(QString("%1/assetsCompiled/%2.rcc").arg(appBin).arg(qrcName));
            assert(registered);
        }
    }

    if (options.contains(Option::ResourceLibrary)) {
        ResourceLibrary::ResourceLibraryPathList pathList;
        {
            ProfilerScope scope("ResourceLibrary search");
            pathList     = ResourceLibrary::searchIndexInFolderRecursive(m_impl->localDataRoot / "Resources");
            auto listCfg = ResourceLibrary::searchIndexInFolderRecursive(m_impl->appBinRoot / "gameResources");
            pathList.append(listCfg);
            pathList.topoSort();
        }
        {
            ProfilerScope scope("ResourceLibrary load");
            m_resourceLibrary = ResourceLibrary::makeMergedLibrary(pathList);
            auto dbIds        = m_resourceLibrary->getDatabaseIds();
            for (auto id : dbIds)
                m_impl->appConfig->globalMutable().databaseItems << QString::fromStdString(id);
        }
    }
    if (options.contains(Option::RNG) || options.contains(Option::MusicBox)) {
        m_randomGeneratorFactory = std::make_shared<RandomGeneratorFactory>();
        m_randomGeneratorUi      = m_randomGeneratorFactory->create();
        m_randomGeneratorUi->makeGoodSeed();
    }

    if (options.contains(Option::GraphicsLibrary))
        m_graphicsLibrary = std::make_shared<GraphicsLibrary>(*m_resourceLibrary);

    if (options.contains(Option::CursorLibrary))
        m_cursorLibrary = std::make_shared<CursorLibrary>(*m_graphicsLibrary);

    if (options.contains(Option::MusicBox)) {
        auto box   = std::make_shared<Sound::MusicBox>(*m_randomGeneratorUi, *m_resourceLibrary);
        m_musicBox = box;
        m_musicBox->setMusicVolume(m_impl->appConfig->sound().musicVolumePercent);
        m_musicBox->setEffectsVolume(m_impl->appConfig->sound().effectsVolumePercent);
        QObject::connect(m_impl->appConfig.get(), &AppSettings::setMusicVolume, box.get(), &Sound::MusicBox::setMusicVolume);
        QObject::connect(m_impl->appConfig.get(), &AppSettings::setEffectsVolume, box.get(), &Sound::MusicBox::setEffectsVolume);
    }

    if (options.contains(Option::GameDatabase)) {
        ProfilerScope scope("GameDatabase load");
        m_gameDatabase = std::make_shared<Core::GameDatabase>(m_impl->appConfig->global().databaseId.toStdString(), *m_resourceLibrary);
    }

    if (options.contains(Option::Translations)) {
        m_impl->appConfig->globalMutable().localeItems << "en_US"
                                                       << "ru_RU";
        QString       currentLocale = m_impl->appConfig->global().localeId;
        ProfilerScope scope("localization");

        m_impl->locEn.reset(new LocalizationManager("en_US", *m_resourceLibrary));
        QApplication::installTranslator(m_impl->locEn.get());

        if (currentLocale != "en_US") {
            m_impl->locMain.reset(new LocalizationManager(currentLocale, *m_resourceLibrary));
            QApplication::installTranslator(m_impl->locMain.get());
        }
    }

#ifdef SHOW_RESOURCES_DEBUG
    QDirIterator it(":/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
#endif

    if (options.contains(Option::QtTranslations)) {
        QStringList moduleNames;
        if (!moduleName.empty())
            moduleNames << QString::fromStdString(moduleName);
        moduleNames << "FreeHeroesCore";
        QString currentLocale = m_impl->appConfig->global().localeId;
        for (const auto& name : moduleNames)
            m_impl->addTranslator(QString(":/Translations/%1_%2.qm").arg(name).arg(currentLocale));
    }

    if (options.contains(Option::LibraryModels)) {
        ProfilerScope scope("LibraryModels load");
        m_modelsProvider = std::make_shared<LibraryModelsProvider>(*m_gameDatabase, *m_musicBox, *m_graphicsLibrary);
    }

    if (options.contains(Option::AppStyle)) {
        qApp->setStyle(new GoldenStyle);
        QApplication::setPalette(QApplication::style()->standardPalette());
    }
    Logger(Logger::Info) << "Application::load - end";
}

IAppSettings& Application::getAppSettings()
{
    return *m_impl->appConfig.get();
}

}
