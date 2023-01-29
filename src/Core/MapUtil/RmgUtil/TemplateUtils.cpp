/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TemplateUtils.hpp"

namespace FreeHeroes {

static_assert(intSqrt(0) == 0);
static_assert(intSqrt(1) == 1);
static_assert(intSqrt(2) == 1);
static_assert(intSqrt(3) == 1);
static_assert(intSqrt(4) == 2);
static_assert(intSqrt(5) == 2);
static_assert(intSqrt(6) == 2);
static_assert(intSqrt(7) == 2);
static_assert(intSqrt(8) == 2);
static_assert(intSqrt(9) == 3);
static_assert(intSqrt(10) == 3);
static_assert(intSqrt(15) == 3);
static_assert(intSqrt(16) == 4);
static_assert(intSqrt(17) == 4);
static_assert(intSqrt(255) == 15);
static_assert(intSqrt(256) == 16);
static_assert(intSqrt(257) == 16);

}
