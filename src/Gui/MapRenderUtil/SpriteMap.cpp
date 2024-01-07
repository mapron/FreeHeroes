/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpriteMap.hpp"

#include "FHPos.hpp"

#ifndef DISABLE_QT
#include <QObject>
#endif

namespace FreeHeroes {

#ifndef DISABLE_QT

std::string SpriteMap::layerTypeToString(Layer layer)
{
    switch (layer) {
        case Layer::Terrain:
            return QObject::tr("Terrain").toStdString();
            break;
        case Layer::Town:
            return QObject::tr("Town").toStdString();
            break;
        case Layer::Hero:
            return QObject::tr("Hero").toStdString();
            break;
        case Layer::Resource:
            return QObject::tr("Resource").toStdString();
            break;
        case Layer::Artifact:
            return QObject::tr("Artifact").toStdString();
            break;
        case Layer::Monster:
            return QObject::tr("Monster").toStdString();
            break;
        case Layer::Dwelling:
            return QObject::tr("Dwelling").toStdString();
            break;
        case Layer::Bank:
            return QObject::tr("Bank").toStdString();
            break;
        case Layer::Mine:
            return QObject::tr("Mine").toStdString();
            break;
        case Layer::Pandora:
            return QObject::tr("Pandora").toStdString();
            break;
        case Layer::Shrine:
            return QObject::tr("Shrine").toStdString();
            break;
        case Layer::SkillHut:
            return QObject::tr("SkillHut").toStdString();
            break;
        case Layer::Scholar:
            return QObject::tr("Scholar").toStdString();
            break;
        case Layer::QuestHut:
            return QObject::tr("QuestHut").toStdString();
            break;
        case Layer::QuestGuard:
            return QObject::tr("QuestGuard").toStdString();
            break;
        case Layer::GeneralVisitable:
            return QObject::tr("GeneralVisitable").toStdString();
            break;
        case Layer::Decoration:
            return QObject::tr("Decoration").toStdString();
            break;
        case Layer::Event:
            return QObject::tr("Event").toStdString();
            break;
        case Layer::Invalid:
        default:

            return QObject::tr("Invalid").toStdString();
            break;
    }
}

#endif

bool SpriteRenderSettings::isFilteredOut(const FHPos& pos) const
{
    if (!m_useRenderWindow)
        return false;
    if (pos.m_z != m_z)
        return true;
    if (pos.m_x < m_xMin || pos.m_x > m_xMax)
        return true;
    if (pos.m_y < m_yMin || pos.m_y > m_yMax)
        return true;
    return false;
}

}
