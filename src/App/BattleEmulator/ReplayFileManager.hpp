/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include <QAbstractListModel>

namespace FreeHeroes::BattleEmulator {

class ReplayFileManager : public QAbstractListModel {
    Core::std_path m_replayRoot;

public:
    struct Record {
        QString        displayName;
        Core::std_path battleReplay;
    };
    std::vector<Record> m_records;

    void setRoot(Core::std_path replayRoot);
    void load();
    void clearAll();

    QString makeNewUniqueName() const;
    Record  makeNewUnique() const;
    void    add(const Record& rec);
    void    deleteFile(const Record& rec);

    int      rowCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    void     renameCurrent(int index, QString s);
};

}
