/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FileIOUtils.hpp"

#include <fstream>

namespace FreeHeroes::Core {

bool readFileIntoBuffer(const std_path& filename, std::string& buffer)
{
    std::ifstream ifs((filename), std::ios_base::in | std::ios_base::binary);
    if (!ifs)
        return false;

    ifs.seekg(0, std::ios::end);

    buffer.resize(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(buffer.data(), buffer.size());
    return true;
}

bool writeFileFromBuffer(const std_path& filename, const std::string& buffer)
{
    std::ofstream ofs((filename), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if (!ofs)
        return false;

    if (buffer.empty())
        return true;

    ofs.write(buffer.data(), buffer.size());
    if (!ofs)
        return false;
    return true;
}

}
