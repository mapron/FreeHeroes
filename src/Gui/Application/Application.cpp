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
#include "MernelPlatform/Profiler.hpp"
#include "AppSettings.hpp"
#include "LibraryModels.hpp"

// Core
#include "CoreApplication.hpp"
#include "MernelPlatform/Logger.hpp"

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
using namespace Mernel;

using namespace Core;

namespace {

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
    CoreApplication*                          coreApp;
    std::unique_ptr<LocalizationManager>      locEn;
    std::unique_ptr<LocalizationManager>      locMain;
    std::vector<std::unique_ptr<QTranslator>> translators;

    QString                      appConfigIniPath;
    std::unique_ptr<AppSettings> appConfig;
    QString                      extraTs;

    void addTranslator(const QString& qmPath)
    {
        auto tr = std::make_unique<QTranslator>();
        tr->load(qmPath);
        QApplication::installTranslator(tr.get());
        translators.emplace_back(std::move(tr));
    }
};

Application::Application(CoreApplication*   coreApp,
                         std::set<Option>   options,
                         const std::string& tsExtraModule)
    : m_impl(new Impl)
    , m_options(std::move(options))
{
    m_impl->coreApp = coreApp;
    m_impl->extraTs = QString::fromStdString(tsExtraModule);
    QApplication::setApplicationName(QString::fromUtf8(CoreApplication::getAppFolder()));

    m_impl->appConfigIniPath = QString::fromStdString(path2string(coreApp->getLocations().getAppdataDir() / "AppConfig.ini"));
    m_impl->appConfig        = std::make_unique<Gui::AppSettings>(m_impl->appConfigIniPath);

    m_impl->appConfig->load();
    QString currentLocale = m_impl->appConfig->global().localeId;
    if (currentLocale.isEmpty())
        currentLocale = QLocale::system().name();
    m_impl->appConfig->globalMutable().localeId = currentLocale;
    // to make user happy just edit values, not to guess keys. Do it earlier in case of crash later.
    m_impl->appConfig->save();

    m_impl->appConfig->globalMutable().databaseItems << g_database_HOTA << g_database_SOD;
}

Application::~Application()
{
    m_impl->appConfig->save();
}

bool Application::load()
{
    {
        m_impl->coreApp->initLogger(m_impl->appConfig->global().logLevel);
        qInstallMessageHandler(loggerQtOutput);
        Logger(Logger::Notice) << "Application started. Log level is " << m_impl->appConfig->global().logLevel;
        qWarning() << "checking Qt logs";
    }

    if (!m_impl->coreApp->load())
        return false;
    if (m_impl->coreApp->hasDatabases())
        m_gameDatabaseUi = m_impl->coreApp->getDatabaseContainer()->getDatabase(m_impl->appConfig->global().databaseId.toStdString());

    Logger(Logger::Info) << "Application::load - start";
    QString       binDir = QString::fromStdString(path2string(m_impl->coreApp->getLocations().getBinDir()));
    ProfilerScope scopeAll("Application::load");
    if (m_options.contains(Option::QtTranslations)) {
        ProfilerScope scope("Qt init resource");
        QStringList   qrc{ "Application", "Battle", "Translations" };
        for (QString qrcName : qrc) {
            [[maybe_unused]] bool registered = QResource::registerResource(QString("%1/assetsCompiled/%2.rcc").arg(binDir).arg(qrcName));
            assert(registered);
        }
    }

    if (m_options.contains(Option::GraphicsLibrary))
        m_graphicsLibrary = std::make_shared<GraphicsLibrary>(m_impl->coreApp->getResourceLibrary());

    if (m_options.contains(Option::CursorLibrary))
        m_cursorLibrary = std::make_shared<CursorLibrary>(m_graphicsLibrary.get());

    if (m_options.contains(Option::MusicBox)) {
        auto randomGeneratorUi = m_impl->coreApp->getRandomGeneratorFactory()->create();
        randomGeneratorUi->makeGoodSeed();
        auto box   = std::make_shared<Sound::MusicBox>(std::move(randomGeneratorUi), m_impl->coreApp->getResourceLibrary());
        m_musicBox = box;
        m_musicBox->setMusicVolume(m_impl->appConfig->sound().musicVolumePercent);
        m_musicBox->setEffectsVolume(m_impl->appConfig->sound().effectsVolumePercent);
        QObject::connect(m_impl->appConfig.get(), &AppSettings::setMusicVolume, box.get(), &Sound::MusicBox::setMusicVolume);
        QObject::connect(m_impl->appConfig.get(), &AppSettings::setEffectsVolume, box.get(), &Sound::MusicBox::setEffectsVolume);
    }

    if (m_options.contains(Option::Translations)) {
        m_impl->appConfig->globalMutable().localeItems << "en_US"
                                                       << "ru_RU"
                                                       << "de_DE";
        QString       currentLocale = m_impl->appConfig->global().localeId;
        ProfilerScope scope("localization");

        m_impl->locEn.reset(new LocalizationManager("en_US", m_impl->coreApp->getResourceLibrary()));
        QApplication::installTranslator(m_impl->locEn.get());

        if (currentLocale != "en_US") {
            m_impl->locMain.reset(new LocalizationManager(currentLocale, m_impl->coreApp->getResourceLibrary()));
            QApplication::installTranslator(m_impl->locMain.get());
        }
    }

#ifdef SHOW_RESOURCES_DEBUG
    QDirIterator it(":/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
#endif

    if (m_options.contains(Option::QtTranslations)) {
        QStringList moduleNames;
        if (!m_impl->extraTs.isEmpty())
            moduleNames << m_impl->extraTs;
        moduleNames << "FreeHeroesCore";
        QString currentLocale = m_impl->appConfig->global().localeId;
        for (const auto& name : moduleNames)
            m_impl->addTranslator(QString(":/Translations/%1_%2.qm").arg(name).arg(currentLocale));
    }

    if (m_options.contains(Option::LibraryModels)) {
        ProfilerScope scope("LibraryModels load");
        m_modelsProvider = std::make_shared<LibraryModelsProvider>(m_gameDatabaseUi, m_musicBox.get(), m_graphicsLibrary.get(), m_impl->appConfig.get());
    }

    if (m_options.contains(Option::AppStyle)) {
        qApp->setStyle(new GoldenStyle);
        QApplication::setPalette(QApplication::style()->standardPalette());
    }
    Logger(Logger::Info) << "Application::load - end";
    return true;
}

IAppSettings* Application::getAppSettings()
{
    return m_impl->appConfig.get();
}

}
