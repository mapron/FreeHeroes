/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "CorePlatformExport.hpp"

namespace FreeHeroes {

class COREPLATFORM_EXPORT AbstractCommandLine {
    using ArgList = std::vector<std::string>;
    using NameSet = std::set<std::string>;

public:
    AbstractCommandLine(NameSet singleValue, NameSet multiValue = {});

    void markRequired(NameSet required);

    std::string getArg(const std::string& key, std::string def = {}) const noexcept;
    ArgList     getMultiArg(const std::string& key) const noexcept;

    bool parseArgs(std::ostream& logStream, int argc, char** argv);
    bool parseArgs(std::ostream& logStream, const std::vector<std::string>& commandLineArgs);
    bool parseArgs(std::ostream& logStream, const std::map<std::string, std::string>& commandLineArgs);

    std::string getHelp() const noexcept;

private:
    const NameSet                      m_options;
    const NameSet                      m_multiOptions;
    NameSet                            m_requiredOptions;
    std::map<std::string, std::string> m_args;
    std::map<std::string, ArgList>     m_multiArgs;
};

}
