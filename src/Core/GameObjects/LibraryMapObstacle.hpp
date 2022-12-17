/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include <string>
#include <vector>

namespace FreeHeroes::Core {

struct LibraryMapObstacle {
    int         legacyId = -1;
    std::string id;

    enum class Type
    {
        Invalid,

        BRUSH,           // 114 165
        BUSH,            // 115  166
        CACTUS,          // 116 167
        CANYON,          // 117 168
        CRATER,          // 118 169
        DEAD_VEGETATION, // 119 170
        FLOWERS,         // 120 171
        FROZEN_LAKE,     // 121 172
        HEDGE,           // 122 173
        HILL,            // 123 174

        HOLE,        // 124 175
        KELP,        // 125 176
        LAKE,        // 126 177
        LAVA_FLOW,   // 127 178
        LAVA_LAKE,   // 128 179
        MUSHROOMS,   // 129 180
        LOG,         // 130 181
        MANDRAKE,    // 131 182
        MOSS,        // 132 183
        MOUND,       // 133 184
        MOUNTAIN,    // 134 185
        OAK_TREES,   // 135 186
        OUTCROPPING, // 136 187
        PINE_TREES,  // 137 188
        PLANT,       // 138 189

        NON_BLOCKING_DECORATION, // 139 140,

        RIVER_DELTA, // 143 190

        ROCK,          // 147 191
        SAND_DUNE,     // 148 192
        SAND_PIT,      // 149 193
        SHRUB,         // 150 194
        SKULL,         // 151 195
        STALAGMITE,    // 152 196
        STUMP,         // 153 197
        TAR_PIT,       // 154 198
        TREES,         // 155 199
        VINE,          // 156 200
        VOLCANIC_VENT, // 157 201
        VOLCANO,       // 158 202
        WILLOW_TREES,  // 159 203
        YUCCA_TREES,   // 160 204
        REEF,          // 161 205

        DESERT_HILLS, // 206
        DIRT_HILLS,   // 207
        GRASS_HILLS,  // 208
        ROUGH_HILLS,  // 209

        SUBTERRANEAN_ROCKS, // 210
        SWAMP_FOLIAGE,      //211

        CLOVER_FIELD,    //  222
        CURSED_GROUND,   // 21 223
        EVIL_FOG,        //  224
        FAVORABLE_WINDS, //  225
        FIERY_FIELDS,    //  226
        HOLY_GROUNDS,    //  227
        LUCID_POOLS,     //  228
        MAGIC_CLOUDS,    //  229
        MAGIC_PLAINS,    // 46 230
        ROCKLANDS,       //  231
    };

    Type type = Type::Invalid;

    LibraryObjectDefConstPtr mapObjectDef = nullptr;
};

}
