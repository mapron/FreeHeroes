/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattleField.hpp"

#include "CoreLogicExport.hpp"

namespace FreeHeroes::Core {

class CORELOGIC_EXPORT BattleFieldPathFinder {
    struct Matrix {
        using value_t = std::int_fast16_t;
        std::vector<value_t> data;
        const int width;
        const int height;
        Matrix(int width, int height): width(width), height(height) {
            data.resize(width * height);
        }
        void set(const BattlePosition pos, value_t val) {
            data[pos.y * width + pos.x] = val;
        }
        void setChecked(const BattlePosition pos, value_t val) {
            if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height) return;
            data[pos.y * width + pos.x] = val;
        }

        value_t get(const BattlePosition pos) const {
            return data[pos.y * width + pos.x];
        }
        value_t getChecked(const BattlePosition pos) const {
            if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height) return -1;
            return data[pos.y * width + pos.x];
        }
        void fill(value_t val) {
            std::fill(data.begin(), data.end(), val);
        }
    };
    const BattleFieldGeometry field;
    Matrix matrix;
    bool goThroughObstacles = false;
    static constexpr std::int_fast16_t valEmpty = -1;
    BattlePositionSet obstacles;


public:
    BattleFieldPathFinder(const BattleFieldGeometry & field) : field(field), matrix(field.width, field.height) {
    }
    void setObstacles(BattlePositionSet ob) { obstacles = std::move(ob); }
    void setGoThroughObstacles(bool goThrough) { goThroughObstacles = goThrough; }
    void floodFill(const BattlePosition start);

    [[nodiscard]] int distanceTo(const BattlePosition end) const { return matrix.get(end); }
    BattlePositionPath fromStartTo(const BattlePosition end, int limit = -1) const;
    BattlePositionSet findAvailable(int limit = -1) const;
    BattlePositionDistanceMap findDistances(int limit = -1) const;
};

}
