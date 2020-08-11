/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace FreeHeroes::Core {

inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return ch < 0 || !std::isspace(ch);
    }));
}
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return ch < 0 || !std::isspace(ch);
    }).base(), s.end());
}

inline std::vector<std::string> splitLine(const std::string & line, char sep, bool skipEmpty = false) {
    std::vector<std::string> result;
    std::string token;
    std::istringstream ss(line);
    while(std::getline(ss, token, sep)) {
        ltrim(token);
        rtrim(token);
        if (!token.empty() || !skipEmpty)
            result.push_back(token);
    }
    return result;
}

}
