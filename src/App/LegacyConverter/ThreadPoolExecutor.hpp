/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QObject>

#include <deque>
#include <thread>
#include <functional>
#include <mutex>
#include <chrono>


namespace FreeHeroes::Conversion {

class ThreadPoolExecutor : public QObject
{
    Q_OBJECT
public:
    using Task = std::function<void()>;

public:
    ThreadPoolExecutor(QObject * parent = nullptr);
    ~ThreadPoolExecutor();

    int getQueueSize() const;
    void add(Task task);
    void start(std::chrono::milliseconds progressInterval);

signals:
    void finished();
    void finishedInternal();
    void progress(int done);
    void progressInternal(int done);

private:
    void threadFunc();
    void clear();
    Task takeTask();
    void checkForProgress();
    void onThreadFinished();

private:
    std::deque<Task> m_queue;
    std::mutex m_queueMutex;
    std::mutex m_threadsMutex;
    std::vector<std::thread> m_threads;
    std::chrono::milliseconds m_nextSendTime {0};
    std::chrono::milliseconds m_progressInterval {0};
    int m_done = 0;
    size_t m_threadsLeft {0};
};


}
