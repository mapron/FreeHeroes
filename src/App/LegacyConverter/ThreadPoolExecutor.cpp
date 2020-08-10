/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ThreadPoolExecutor.hpp"

namespace FreeHeroes::Conversion {

namespace {
std::chrono::milliseconds curMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
}
}

ThreadPoolExecutor::ThreadPoolExecutor(QObject * parent)
    : QObject(parent)
{
    connect(this, &ThreadPoolExecutor::finishedInternal, this, &ThreadPoolExecutor::finished, Qt::QueuedConnection);
    connect(this, &ThreadPoolExecutor::progressInternal, this, &ThreadPoolExecutor::progress, Qt::QueuedConnection);
}

ThreadPoolExecutor::~ThreadPoolExecutor()
{
    clear();
}

int ThreadPoolExecutor::getQueueSize() const
{
    return static_cast<int>(m_queue.size());
}

void ThreadPoolExecutor::add(ThreadPoolExecutor::Task task)
{
    std::unique_lock lock(m_queueMutex);
    Q_ASSERT(!!task);
    m_queue.push_back(task);
}

void ThreadPoolExecutor::start(std::chrono::milliseconds progressInterval)
{
    //clear();
    m_progressInterval = progressInterval;
    m_nextSendTime = std::chrono::milliseconds{0};
    const size_t threadCount = std::thread::hardware_concurrency();
    m_threadsLeft = threadCount;
    for (size_t i=0; i< threadCount; ++i) {
        m_threads.emplace_back([this]{ threadFunc(); });
    }
}

void ThreadPoolExecutor::threadFunc()
{
    while (true) {
        auto task = takeTask();
        if (!task)
            break;
        task();
    }
    onThreadFinished();
}

void ThreadPoolExecutor::clear()
{
    {
        std::unique_lock lock(m_queueMutex);
        m_queue.clear();
        m_done = 0;
    }
    for (auto & thread : m_threads) {
        if (thread.joinable())
            thread.join();
    }
}

ThreadPoolExecutor::Task ThreadPoolExecutor::takeTask()
{
    std::unique_lock lock(m_queueMutex);
    if (m_queue.empty()) {
        return {};
    }
    Task task = m_queue.front();
    m_queue.pop_front();
    m_done++;
    checkForProgress();

    Q_ASSERT(!!task);
    return task;
}

void ThreadPoolExecutor::checkForProgress()
{
    auto cur = curMS();
    if (cur < m_nextSendTime)
        return;
    m_nextSendTime = cur + m_progressInterval;
    emit progress(m_done);
}

void ThreadPoolExecutor::onThreadFinished()
{
    std::unique_lock lock(m_threadsMutex);
    if (m_threadsLeft > 0)
        m_threadsLeft--;

    if (m_threadsLeft > 0)
        return;

    emit finishedInternal();
}

}
