/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LocalizationManager.hpp"

#include <fstream>
#include <sstream>

namespace FreeHeroes::Gui {

using namespace Core;

namespace {

// English
// 0 - singular, 1 - plural
// default is plural.
int pluralSelector_EN(int n, int availableSize)
{
    assert(availableSize > 1);
    assert(n >= -1);
    if (n == -1) // default
        return 1;

    if (n == 1)
        return 0;
    return 1;
}

// Polish
// 0 - singular, 1 - plural, [2 - 2..4 ] [3 - 5+ ]
// default is plural.
int pluralSelector_PL(int n, int availableSize)
{
    assert(availableSize > 1);
    assert(n >= -1);
    if (n == -1) // default
        return 1;
    if (n == 1)
        return 0;
    if ((n % 10) >= 2 && (n % 10) <= 4 && ((n % 100 < 10) || (n % 100 > 20)))
        return availableSize > 2 ? 2 : 1;

    return availableSize > 3 ? 3 : 1;
}

// Polish
// 0 - singular, 1 - plural, [2 - 2..4 ] [3 - 5+ ]
// default is plural.
int pluralSelector_RU(int n, int availableSize)
{
    assert(availableSize > 1);
    assert(n >= -1);
    if (n == -1) // default
        return 1;
    if (n % 10 == 1 && n % 100 != 11)
        return 0;

    if ((n % 10) >= 2 && (n % 10) <= 4 && ((n % 100 < 10) || (n % 100 > 20)))
        return availableSize > 2 ? 2 : 1;

    return availableSize > 3 ? 3 : 1;
}

std::vector<std::string> makeAccusativeForms(const std::string& locale, std::vector<std::string> source)
{
    if (locale == "ru_RU" && source.size() >= 4) {
        source[0] = source[4];
    }
    return source;
}

}

LocalizationManager::LocalizationManager(QString                 localeId,
                                         const IResourceLibrary* resourceLibrary)
    : m_resourceLibrary(resourceLibrary)
    , m_mainLocaleId(localeId.toStdString())
{
    // @todo: more language support.
    // https://doc.qt.io/qt-5/i18n-plural-rules.html
    m_pluralSelector = localeId.startsWith("ru_") ? pluralSelector_RU : localeId.startsWith("pl_") ? pluralSelector_PL
                                                                                                   : pluralSelector_EN;

    for (const auto& id : resourceLibrary->getTranslationContextChildren(m_mainLocaleId, "units")) {
        auto&                    unitMap = m_records["units"];
        std::vector<std::string> res     = resourceLibrary->getTranslation(m_mainLocaleId, "units", id);
        unitMap[id + ".accusative"]      = makeAccusativeForms(localeId.toStdString(), res);
    }
}

QString LocalizationManager::translate(const char* context, const char* sourceText, const char* disambiguation, int n) const
{
    (void) disambiguation;
    QString str = translateByRecords(context, sourceText, n);
    if (!str.isEmpty())
        return str;
    return translateByLibrary(context, sourceText, n);
}

bool LocalizationManager::isEmpty() const
{
    return false;
}

QString LocalizationManager::translateByRecords(const char* context, const char* sourceText, int n) const
{
    auto contextIt = m_records.find(context);
    if (contextIt == m_records.cend())
        return {};

    const KeyToPluralsMapping& contextMap = *contextIt;
    auto                       keyIt      = contextMap.find(sourceText);
    if (keyIt == contextMap.cend())
        return {};

    const std::vector<std::string>& values = *keyIt;
    if (values.empty())
        return {};

    const std::string& value = values[values.size() == 1 ? 0 : m_pluralSelector(n, values.size())];
    return QString::fromStdString(value);
}

QString LocalizationManager::translateByLibrary(const char* context, const char* sourceText, int n) const
{
    std::vector<std::string> values = m_resourceLibrary->getTranslation(m_mainLocaleId, context, sourceText);
    if (values.empty())
        return {};
    const std::string& value = values[values.size() == 1 ? 0 : m_pluralSelector(n, values.size())];
    return QString::fromStdString(value);
}

}
