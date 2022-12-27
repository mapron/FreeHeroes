/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpriteMap.hpp"

namespace FreeHeroes {

QString SpriteMap::layerTypeToString(Layer layer)
{
    switch (layer) {
        case Layer::Terrain:
            return QObject::tr("Terrain");
            break;
        case Layer::Town:
            return QObject::tr("Town");
            break;
        case Layer::Hero:
            return QObject::tr("Hero");
            break;
        case Layer::Resource:
            return QObject::tr("Resource");
            break;
        case Layer::Artifact:
            return QObject::tr("Artifact");
            break;
        case Layer::Monster:
            return QObject::tr("Monster");
            break;
        case Layer::Dwelling:
            return QObject::tr("Dwelling");
            break;
        case Layer::Bank:
            return QObject::tr("Bank");
            break;
        case Layer::Mine:
            return QObject::tr("Mine");
            break;
        case Layer::Pandora:
            return QObject::tr("Pandora");
            break;
        case Layer::Shrine:
            return QObject::tr("Shrine");
            break;
        case Layer::SkillHut:
            return QObject::tr("SkillHut");
            break;
        case Layer::Scholar:
            return QObject::tr("Scholar");
            break;
        case Layer::QuestHut:
            return QObject::tr("QuestHut");
            break;
        case Layer::GeneralVisitable:
            return QObject::tr("GeneralVisitable");
            break;
        case Layer::Decoration:
            return QObject::tr("Decoration");
            break;

        case Layer::Invalid:
        default:

            return QObject::tr("Invalid");
            break;
    }
}

}
