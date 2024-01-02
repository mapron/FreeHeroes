/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FH2H3M.hpp"
#include "H3M2FH.hpp"

#include "H3MConversion.hpp"

namespace FreeHeroes {

void convertH3M2FH(const H3Map& src, FHMap& dest)
{
    H3M2FHConverter converter(dest.m_database);
    converter.convertMap(src, dest);
}

void convertFH2H3M(const FHMap& src, H3Map& dest)
{
    FH2H3MConverter converter(src.m_database);
    converter.convertMap(src, dest);
}

}
