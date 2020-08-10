/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <filesystem>

namespace FreeHeroes::Core {

namespace std_fs = std::filesystem;
using std_path = std_fs::path;

inline std::string path2string(const std_path & path) {
    auto str = path.u8string();
    std::string result;
    result.resize(str.size());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(str[i]);
    }
    return result;
}

inline std_path string2path(const std::string & str) {
    std::u8string result;
    //auto str = path.u8string();
    result.resize(str.size());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char8_t>(str[i]);
    }
    return result;
}

}
