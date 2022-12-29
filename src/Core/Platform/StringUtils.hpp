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

inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                return ch < 0 || !std::isspace(ch);
            }));
}
inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
                return ch < 0 || !std::isspace(ch);
            }).base(),
            s.end());
}

inline std::vector<std::string> splitLine(const std::string& line, char sep, bool skipEmpty = false)
{
    std::vector<std::string> result;
    std::string              token;
    std::istringstream       ss(line);
    while (std::getline(ss, token, sep)) {
        ltrim(token);
        rtrim(token);
        if (!token.empty() || !skipEmpty)
            result.push_back(token);
    }
    return result;
}

inline std::vector<std::string> splitLine(std::string line, const std::string& delimiter, bool skipEmpty = false)
{
    std::vector<std::string> result;
    size_t                   pos = 0;
    std::string              token;
    while ((pos = line.find(delimiter)) != std::string::npos) {
        token = line.substr(0, pos);
        result.push_back(token);
        line.erase(0, pos + delimiter.length());
    }
    result.push_back(line);
    return result;
}

inline std::string joinString(const std::vector<std::string>& parts, const std::string& glue)
{
    std::string result;
    bool        started = false;
    for (const auto& part : parts) {
        if (started)
            result += glue;
        result += part;
        started = true;
    }
    return result;
}

}
