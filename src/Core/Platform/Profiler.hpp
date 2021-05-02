/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CorePlatformExport.hpp"

#include <string>
#include <string_view>
#include <memory>

#include <cstdint>

namespace FreeHeroes {

class COREPLATFORM_EXPORT ProfilerContext {
    friend class ProfilerScope;

public:
    ProfilerContext();
    ~ProfilerContext();

    void addRecord(std::string_view key, int64_t ms);
    void clearAll();

    void        printToStdErr() const;
    std::string printToStr() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class COREPLATFORM_EXPORT ScopeTimer {
    int64_t  start;
    int64_t* out = nullptr;

public:
    ScopeTimer();
    explicit ScopeTimer(int64_t& out);
    ~ScopeTimer();
    int64_t elapsed() const noexcept;
};

class COREPLATFORM_EXPORT ProfilerScope {
    ScopeTimer       timer;
    std::string_view key;
    ProfilerContext* context = nullptr;

public:
    // construct in global context (if this is enabled by macro, otherwise do nothing)
    explicit ProfilerScope(std::string_view key);
    // write data in user provided statistic context
    ProfilerScope(std::string_view key, ProfilerContext& customContext);

    ~ProfilerScope();

    // call corresponding from global context or do nothing.
    static void        addRecord(std::string_view key, int64_t ms);
    static void        printToStdErr();
    static void        clearAll();
    static std::string printToStr();
};

class COREPLATFORM_EXPORT ProfilerDefaultContextSwitcher {
    ProfilerContext* contextPrev = nullptr;
    bool             isTrivial   = false;

public:
    ProfilerDefaultContextSwitcher(ProfilerContext& customContext) noexcept;
    ~ProfilerDefaultContextSwitcher();
};

}
