/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiLocalizationExport.hpp"

#include "FsUtils.hpp"
#include "IResourceLibrary.hpp"


#include <QTranslator>
#include <QMap>
#include <QVector>

#include <functional>

namespace FreeHeroes::Gui {

class GUILOCALIZATION_EXPORT LocalizationManager : public QTranslator
{
public:
    LocalizationManager(QString mainLocaleId, Core::IResourceLibrary & resourceLibrary);

    // QTranslator interface
public:
    QString translate(const char* context, const char* sourceText, const char* disambiguation, int n) const override;
    bool isEmpty() const override;

private:
    using KeyToPluralsMapping = QMap<std::string, std::vector<std::string>>;

private:
    QString translateByRecords(const char* context, const char* sourceText, int n) const;
    QString translateByLibrary(const char* context, const char* sourceText, int n) const;

private:
    using LocaleMap = QMap<std::string, KeyToPluralsMapping>;
    LocaleMap m_records;
    Core::IResourceLibrary & m_resourceLibrary;

    std::function<int(int n, int availableSize)> m_pluralSelector;

    const std::string m_mainLocaleId;
    //const QStringList m_allLocales;
};

}
