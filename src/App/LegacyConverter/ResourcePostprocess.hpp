/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IResourceLibrary.hpp"

namespace FreeHeroes::Conversion {

class ResourcePostprocess
{
public:

    void concatSprites(Core::IResourceLibrary & resources,
                      const std::vector<std::string> &  in,
                      const std::string & out,
                      bool vertical);
};

}
