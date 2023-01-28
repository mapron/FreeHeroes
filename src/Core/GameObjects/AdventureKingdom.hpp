/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryResource.hpp"

namespace FreeHeroes::Core {

struct AdventureKingdom {
    ResourceAmount currentResources;
    ResourceAmount weekIncome;
    ResourceAmount dayIncome;
    struct Date {
        int  month      = 1;
        int  week       = 1;
        int  day        = 1;
        bool monthStart = true;
        bool weekStart  = true;
        void next()
        {
            day++;
            weekStart  = false;
            monthStart = false;
            if (day > 7) {
                day = 1;
                week++;
                weekStart = true;
            }
            if (week > 4) {
                week = 1;
                month++;
                monthStart = true;
            }
        }
    };
    Date currentDate;
};

}
