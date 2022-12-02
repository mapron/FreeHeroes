/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FileIOUtils.hpp"

#include <fstream>

namespace FreeHeroes::Core {

bool readFileIntoBuffer(const std_path& filename, std::string& buffer) noexcept(true)
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

bool writeFileFromBuffer(const std_path& filename, const std::string& buffer) noexcept(true)
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

bool readFileIntoHolder(const std_path& filename, ByteArrayHolder& holder) noexcept(true)
{
    std::string buffer;
    if (!readFileIntoBuffer(filename, buffer))
        return false;
    holder.resize(buffer.size());
    memcpy(holder.data(), buffer.data(), holder.size());
    return true;
}

bool writeFileFromHolder(const std_path& filename, const ByteArrayHolder& holder) noexcept(true)
{
    std::string buffer;
    buffer.resize(holder.size());
    memcpy(buffer.data(), holder.data(), holder.size());
    return writeFileFromBuffer(filename, buffer);
}

std::string readFileIntoBufferThrow(const std_path& filename) noexcept(false)
{
    std::string buffer;
    if (!readFileIntoBuffer(filename, buffer))
        throw std::runtime_error("Failed to read file: " + path2string(filename));
    return buffer;
}

void writeFileFromBufferThrow(const std_path& filename, const std::string& buffer) noexcept(false)
{
    if (!writeFileFromBuffer(filename, buffer))
        throw std::runtime_error("Failed to write file: " + path2string(filename));
}

ByteArrayHolder readFileIntoHolderThrow(const std_path& filename) noexcept(false)
{
    ByteArrayHolder holder;
    if (!readFileIntoHolder(filename, holder))
        throw std::runtime_error("Failed to read file: " + path2string(filename));
    return holder;
}

void writeFileFromHolderThrow(const std_path& filename, const ByteArrayHolder& holder) noexcept(false)
{
    if (!writeFileFromHolder(filename, holder))
        throw std::runtime_error("Failed to write file: " + path2string(filename));
}

}
