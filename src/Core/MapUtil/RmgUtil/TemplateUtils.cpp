/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TemplateUtils.hpp"

namespace FreeHeroes {

static_assert(fpsin_fixed(0) == 0);
static_assert(fpsin_fixed(8192) == 4096); // sin(90) : 1
static_assert(fpsin_fixed(4096) == 2896); // sin(45) : sqrt(2)/2 == 0.707 ; 4096 * 0.707 = 2896

static_assert(fpsin_deg_impl(0) == 0);
static_assert(fpsin_deg_impl(1) == 173);    // sin(1) == 0.0175
static_assert(fpsin_deg_impl(5) == 871);    // sin(4) == 0.0872
static_assert(fpsin_deg_impl(15) == 2585);  // sin(15) == 0.2588
static_assert(fpsin_deg_impl(30) == 4997);  // sin(30) == 0.5000
static_assert(fpsin_deg_impl(45) == 7070);  // sin(45) == 0.7071
static_assert(fpsin_deg_impl(60) == 8662);  // sin(60) == 0.8660
static_assert(fpsin_deg_impl(85) == 9960);  // sin(85) == 0.9962
static_assert(fpsin_deg_impl(90) == 10000); // sin(90) : 1

static_assert(fpsin_deg_impl(210) == -4997);

static_assert(fpsin_deg(0) == 0);
static_assert(fpsin_deg(1) == 173);
static_assert(fpsin_deg(5) == 871);
static_assert(fpsin_deg(15) == 2585);
static_assert(fpsin_deg(30) == 5000);
static_assert(fpsin_deg(45) == 7071);
static_assert(fpsin_deg(60) == 8660);
static_assert(fpsin_deg(85) == 9960);
static_assert(fpsin_deg(90) == 10000);

static_assert(fpsin_deg(120) == 8660);
static_assert(fpsin_deg(135) == 7071);
static_assert(fpsin_deg(150) == 5000);
static_assert(fpsin_deg(210) == -5000);
static_assert(fpsin_deg(225) == -7071);
static_assert(fpsin_deg(240) == -8660);
static_assert(fpsin_deg(270) == -10000);

static_assert(fpsin_deg(-135) == -7070);
static_assert(fpsin_deg(-140) == -6428);

static_assert(fpcos_deg(0) == 10000);
static_assert(fpcos_deg(60) == 5000);
static_assert(fpcos_deg(300) == 5000);

static_assert(fpcos_deg(-136) == -7192);

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

static_assert(radiusVector(0) == FHPos{ 10000, 0 });
static_assert(radiusVector(30) == FHPos{ 8660, 5000 });
static_assert(radiusVector(45) == FHPos{ 7071, 7071 });
static_assert(radiusVector(60) == FHPos{ 5000, 8660 });
static_assert(radiusVector(90) == FHPos{ 0, 10000 });
static_assert(radiusVector(135) == FHPos{ -7071, 7071 });

static_assert(radiusVector(225) == FHPos{ -7071, -7071 });
static_assert(radiusVector(-135) == FHPos{ -7070, -7070 });

static_assert(radiusVector(FHPosDirection::R) == FHPos{ 10000, 0 });
static_assert(radiusVector(FHPosDirection::BR) == FHPos{ 7071, 7071 });
static_assert(radiusVector(FHPosDirection::B) == FHPos{ 0, 10000 });
static_assert(radiusVector(FHPosDirection::BL) == FHPos{ -7071, 7071 });
static_assert(radiusVector(FHPosDirection::L) == FHPos{ -10000, 0 });
static_assert(radiusVector(FHPosDirection::TL) == FHPos{ -7071, -7071 });
static_assert(radiusVector(FHPosDirection::T) == FHPos{ 0, -10000 });
static_assert(radiusVector(FHPosDirection::TR) == FHPos{ 7071, -7071 });

}
