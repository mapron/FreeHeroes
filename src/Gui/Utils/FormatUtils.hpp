/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiUtilsExport.hpp"

#include <QString>
#include <QObject>

namespace FreeHeroes::Core {
struct DamageDesc;
}

namespace FreeHeroes::Gui {

class GUIUTILS_EXPORT FormatUtils : public QObject {
    Q_OBJECT
public:
    static QString prepareDescription(QString description);
    static QString formatBattleLog(QString text, bool infoIsWhite);
    static QString battleSidePrefix(bool attacker);


    static QString formatSequenceStr(QString base, QString start, QString cur);


    static QString formatSequenceInt(int base, int start, int cur);
    static QString formatSequenceDmg(Core::DamageDesc base, Core::DamageDesc start, Core::DamageDesc cur);

    static QString formatLargeInt(int64_t value);

};

}
