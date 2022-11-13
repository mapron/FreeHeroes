/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "CommandLineUtils.hpp"
#include "StringUtils.hpp"

#include <iostream>

namespace FreeHeroes {

AbstractCommandLine::AbstractCommandLine(NameSet singleValue, NameSet multiValue)
    : m_options(std::move(singleValue))
    , m_multiOptions(std::move(multiValue))
{
}

void AbstractCommandLine::markRequired(NameSet required)
{
    m_requiredOptions = std::move(required);
}

AbstractCommandLine::ArgList AbstractCommandLine::getMultiArg(const std::string& key) const noexcept
{
    auto it = m_multiArgs.find(key);
    return it == m_multiArgs.cend() ? ArgList{} : it->second;
}

std::string AbstractCommandLine::getArg(const std::string& key, std::string def) const noexcept
{
    auto it = m_args.find(key);
    return it == m_args.cend() ? def : it->second;
}

bool AbstractCommandLine::parseArgs(std::ostream& logStream, int argc, char** argv)
{
    std::vector<std::string> argsVector;
    for (int i = 1; i < argc; ++i)
        argsVector.push_back(argv[i]);
    return parseArgs(logStream, argsVector);
}

bool AbstractCommandLine::parseArgs(std::ostream& logStream, const std::vector<std::string>& commandLineArgs)
{
    std::map<std::string, std::string> argsMap;
    if (commandLineArgs.size() % 2 == 1) {
        logStream << "Arguments count should be even. Argument '" << commandLineArgs.back()
                  << "' provided without option key.\n";
        return false;
    }
    for (size_t i = 0; i < commandLineArgs.size(); i += 2) {
        std::string arg(commandLineArgs[i]);
        if (arg[0] != '-') {
            logStream << "Argument '" << arg << "' provided without option key.\n";
            return false;
        }
        if (arg[1] != '-') {
            logStream << "Argument '" << arg << "' must start with '--'.\n";
            return false;
        }
        argsMap[arg.substr(2)] = commandLineArgs[i + 1];
    }
    return parseArgs(logStream, argsMap);
}

bool AbstractCommandLine::parseArgs(std::ostream& logStream, const std::map<std::string, std::string>& commandLineArgs)
{
    bool result = true;
    for (const auto& [key, value] : commandLineArgs) {
        const bool isMultival  = m_multiOptions.contains(key);
        const bool isSingleval = m_options.contains(key);
        if (isMultival && isSingleval) {
            logStream << "Command line option declared both as single-val and multi-val: '" << key << "'\n";
            result = false;
            continue;
        }
        if (!isMultival && !isSingleval) {
            logStream << "Unknown parameter '" << key << "'\n";
            result = false;
            continue;
        }
        auto stringList = Core::splitLine(value, ',', true);
        if (isSingleval) {
            if (stringList.size() > 1) {
                logStream << "Option '" << key << "' should not have several comma-separated values.\n";
                result = false;
                continue;
            }
            if (m_requiredOptions.contains(key) && value.empty()) {
                logStream << "Option '" << key << "' is required and must not be empty.\n";
                result = false;
                continue;
            }
            m_args[key] = value;
            continue;
        }
        if (m_requiredOptions.contains(key) && stringList.empty()) {
            logStream << "Option '" << key << "' is required and must not be empty.\n";
            result = false;
            continue;
        }
        m_multiArgs[key] = std::move(stringList);
    }
    for (const auto& key : m_requiredOptions) {
        if (!m_args.contains(key) && !m_multiArgs.contains(key)) {
            logStream << "Option '" << key << "' is required.\n";
            result = false;
            continue;
        }
    }
    return result;
}

std::string AbstractCommandLine::getHelp() const noexcept
{
    std::string result = " --option1 value1 [--option2 value2 ] ... , possible options:\n";
    for (const auto& name : m_options) {
        result += "--" + name + " value";
        if (m_requiredOptions.contains(name))
            result += " (REQUIRED)";
        result += "\n";
    }
    for (const auto& name : m_multiOptions) {
        result += "--" + name + " value1[,value2...] (multiple values)";
        if (m_requiredOptions.contains(name))
            result += " (REQUIRED)";
        result += "\n";
    }
    return result;
}

}
