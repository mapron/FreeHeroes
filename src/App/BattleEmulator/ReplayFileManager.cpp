/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ReplayFileManager.hpp"

#include <deque>

#include <QSet>

namespace FreeHeroes::BattleEmulator {
using namespace Core;

void ReplayFileManager::setRoot(std_path replayRoot)
{
    m_replayRoot = replayRoot;
    if (!std_fs::exists(m_replayRoot))
        std_fs::create_directories(m_replayRoot);
}

void ReplayFileManager::load()
{
    beginResetModel();
    std::deque<std_path> files;
    for (auto& it : std_fs::directory_iterator(m_replayRoot)) {
        if (it.is_regular_file()) {
            files.push_back(it.path());
        }
    }

    std::sort(files.begin(), files.end(), [](const std_path& r, const std_path& l) {
        return std_fs::directory_entry(r).last_write_time() < std_fs::directory_entry(l).last_write_time();
    });
    for (auto& f : files) {
        std::string name = path2string(f.filename().stem());
        Record      rec{ QString::fromStdString(name), f };
        m_records.push_back(rec);
    }
    endResetModel();
}

void ReplayFileManager::clearAll()
{
    beginResetModel();
    m_records.clear();
    std::deque<std_path> files;
    for (auto& it : std_fs::directory_iterator(m_replayRoot)) {
        if (it.is_regular_file())
            files.push_back(it.path());
    }
    for (auto& file : files) {
        std::error_code ec;
        std_fs::remove(file, ec);
    }
    endResetModel();
}

QString ReplayFileManager::makeNewUniqueName() const
{
    QSet<QString> existing;
    for (const auto& r : m_records)
        existing << r.displayName;
    QString name = "Replay";
    for (int i = 1; i < 1000000; i++) {
        QString nameTmp = name + QString::number(i);
        if (!existing.contains(nameTmp))
            return nameTmp;
    }
    Q_ASSERT("million replays? how?");
    return "";
}

ReplayFileManager::Record ReplayFileManager::makeNewUnique() const
{
    Record rec;
    rec.displayName  = makeNewUniqueName();
    rec.battleReplay = m_replayRoot / (rec.displayName.toStdString() + ".json");
    return rec;
}

void ReplayFileManager::add(const ReplayFileManager::Record& rec)
{
    beginResetModel();
    m_records.push_back(rec);
    endResetModel();
}

void ReplayFileManager::deleteFile(const ReplayFileManager::Record& rec)
{
    if (std_fs::exists(rec.battleReplay))
        std_fs::remove(rec.battleReplay);
}

int ReplayFileManager::rowCount(const QModelIndex&) const
{
    return m_records.size();
}

QVariant ReplayFileManager::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        return m_records[index.row()].displayName;
    }
    return {};
}

void ReplayFileManager::renameCurrent(int index, QString s)
{
    if (index < 0 || s.isEmpty())
        return;
    auto newFname = m_replayRoot / string2path(s.toStdString() + ".json");
    if (std_fs::exists(newFname))
        return;
    std::error_code ec;
    std_fs::rename(m_records[index].battleReplay, newFname, ec);
    if (ec)
        return;
    m_records[index].displayName  = s;
    m_records[index].battleReplay = newFname;
    emit dataChanged(this->index(index, 0), this->index(index, 0));
}

}
