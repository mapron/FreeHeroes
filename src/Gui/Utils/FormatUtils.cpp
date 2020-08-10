/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FormatUtils.hpp"

#include "Stat.hpp"

namespace FreeHeroes::Gui {

QString FormatUtils::prepareDescription(QString description)
{
    const QString formatTitleBegin = QString("<font color=\"#EDD57A\">");
    const QString formatTitleEnd = QString("</font>");
    if (description.startsWith("\""))
        description = description.mid(1);

    if (description.endsWith("\""))
        description = description.mid(0, description.size() -1);

    description.replace("\\n", "<br>");
    description.replace("{", formatTitleBegin);
    description.replace("}", formatTitleEnd);
    return description;
}

QString FormatUtils::formatBattleLog(QString text, bool infoIsWhite)
{
    text.replace("<d>", "<font color=\"#83E2E2\"><b>");
    text.replace("</d>", "</b></font>");
    text.replace("<k>", "<font color=\"#E08282\"><b>");
    text.replace("</k>", "</b></font>");
    if (infoIsWhite)
        text.replace("<i>", "<font color=\"#ffffff\"><b>");
    else
        text.replace("<i>", "<font color=\"#FAE496\"><b>");
    text.replace("</i>", "</b></font>");
    return text;
}

QString FormatUtils::battleSidePrefix(bool attacker)
{
    if (attacker)
        return QString("<font color=\"#A30004\"><b>") + tr("A") + QString("⮞") + "</b></font> ";
    else
        return QString("<font color=\"#3A35A0\"><b>") + tr("D") + QString("⮞") + "</b></font> ";
}

QString FormatUtils::formatSequenceStr(QString base, QString start, QString cur) {
    if (base == start && start == cur)
        return QString("%1").arg(cur);

    if (base != start && start != cur)
        return QString("<font color=\"#696969\">%1 ⮞ %2 ⮞</font> %3").arg(base).arg(start).arg(cur);

    return QString("<font color=\"#696969\">%1 ⮞</font> %2").arg(base).arg(cur);
}

QString FormatUtils::formatSequenceInt(int base, int start, int cur){
    return formatSequenceStr(QString("%1").arg(base), QString("%1").arg(start), QString("%1").arg(cur));
}

QString FormatUtils::formatSequenceDmg(Core::DamageDesc base, Core::DamageDesc start, Core::DamageDesc cur) {
    auto formatDmg = [](Core::DamageDesc desc) {
        if (desc.minDamage == desc.maxDamage)
            return QString("%1").arg(desc.minDamage);
        return QString("%1 - %2").arg(desc.minDamage).arg(desc.maxDamage);
    };
    return formatSequenceStr(formatDmg(base),
                             formatDmg(start),
                             formatDmg(cur));
}

QString FormatUtils::formatLargeInt(int64_t value) {
    QString suffix = "";
    if (value > 20000) {
        suffix = "K";
        value /= 1000;
    }
    if (value > 20000) {
        suffix = "M";
        value /= 1000;
    }
    return QString("%1%2").arg(value).arg(suffix);
}


}
