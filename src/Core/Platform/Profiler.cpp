/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Profiler.hpp"
#include "Logger.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <map>
#include <deque>

#define FH_ENABLE_GLOBAL_PROFILER_CONTEXT 1

#ifdef NDEBUG
#define FH_ENABLE_PROFILER_LOGGING 1
#endif

namespace FreeHeroes {

struct ProfilerContext::Impl {
    struct Rec {
        int64_t us    = 0;
        int     calls = 0;
        void    add(int64_t v)
        {
            us += v;
            calls++;
        }
    };

    void printTo(std::ostream& os)
    {
        for (auto& p : all) {
            os << p.first << "=" << (p.second.us / 1000) << " #" << (p.second.calls) << "\n";
        }
    }

    void pushPrefix(std::string_view key)
    {
        stackPrefix += std::string{ key } + "->";
        stack.push_back(stackPrefix);
#ifdef FH_ENABLE_PROFILER_LOGGING
        Logger(Logger::Info) << stackPrefix;
#endif
    }
    void addRecord(std::string_view key, int64_t value)
    {
        std::string keyFull = stackPrefix + std::string(key);
        all[std::move(keyFull)].add(value);
#ifdef FH_ENABLE_PROFILER_LOGGING
        Logger(Logger::Info) << "/ " << stackPrefix;
#endif
    }

    void pop(std::string_view key, int64_t value)
    {
        stack.pop_back();
        stackPrefix = stack.empty() ? "" : stack.back();
        addRecord(key, value);
    }

    std::map<std::string, Rec> all{};
    std::deque<std::string>    stack;
    std::string                stackPrefix;
};

ProfilerContext::ProfilerContext()
    : m_impl(std::make_unique<ProfilerContext::Impl>())
{
}

ProfilerContext::~ProfilerContext() = default;

void ProfilerContext::addRecord(std::string_view key, int64_t ms)
{
    m_impl->addRecord(key, ms * 1000);
}
void ProfilerContext::clearAll()
{
    m_impl->all.clear();
    m_impl->stack.clear();
    m_impl->stackPrefix.clear();
}

void ProfilerContext::printToStdErr() const
{
    m_impl->printTo(std::cerr);
}

std::string ProfilerContext::printToStr() const
{
    std::ostringstream ss;
    m_impl->printTo(ss);
    return ss.str();
}

namespace {

int64_t curUS()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

ProfilerContext* defaultContext = nullptr;

ProfilerContext* getGlobalProfilerContext()
{
#ifdef FH_ENABLE_GLOBAL_PROFILER_CONTEXT
    static ProfilerContext globalProfilerContext{};
    return &globalProfilerContext;
#else
    return nullptr;
#endif
}

ProfilerContext* getDefaultContext()
{
    return defaultContext ? defaultContext : getGlobalProfilerContext();
}

}

ScopeTimer::ScopeTimer()
    : start(curUS())
    , out(nullptr)
{
}

ScopeTimer::ScopeTimer(int64_t& out)
    : start(curUS())
    , out(&out)
{
}

ScopeTimer::~ScopeTimer()
{
    if (out)
        *out = elapsed();
}

int64_t ScopeTimer::elapsed() const noexcept
{
    return curUS() - start;
}

ProfilerScope::ProfilerScope(std::string_view key)
    : key(key)
    , context(getDefaultContext())
{
    if (!context)
        return;
    context->m_impl->pushPrefix(key);
}

ProfilerScope::ProfilerScope(std::string_view key, ProfilerContext& customContext)
    : key(key)
    , context(&customContext)
{
    context->m_impl->pushPrefix(key);
}

ProfilerScope::~ProfilerScope()
{
    if (!context)
        return;

    context->m_impl->pop(key, timer.elapsed());
}

void ProfilerScope::printToStdErr()
{
    auto* context = getDefaultContext();
    if (!context)
        return;
    context->printToStdErr();
}

void ProfilerScope::clearAll()
{
    auto* context = getDefaultContext();
    if (!context)
        return;
    context->clearAll();
}

std::string ProfilerScope::printToStr()
{
    auto* context = getDefaultContext();
    if (!context)
        return {};
    return context->printToStr();
}
void ProfilerScope::addRecord(std::string_view str, int64_t ms)
{
    auto* context = getDefaultContext();
    if (!context)
        return;
    context->addRecord(str, ms);
}

ProfilerDefaultContextSwitcher::ProfilerDefaultContextSwitcher(ProfilerContext& customContext) noexcept
{
    isTrivial = &customContext == defaultContext;
    if (isTrivial)
        return;

    contextPrev    = defaultContext;
    defaultContext = &customContext;
}

ProfilerDefaultContextSwitcher::~ProfilerDefaultContextSwitcher()
{
    if (!isTrivial)
        defaultContext = contextPrev;
}

}
