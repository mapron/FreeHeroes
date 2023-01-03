/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"

#include "CoreLogicExport.hpp"

namespace FreeHeroes::Core {
class IResourceLibrary;

class CORELOGIC_EXPORT GameDatabaseContainer : public IGameDatabaseContainer {
public:
    GameDatabaseContainer(const IResourceLibrary* resourceLibrary);
    ~GameDatabaseContainer();

    const IGameDatabase* getDatabase(const DbOrder& dbIndexFilesList) const noexcept override;

    const IGameDatabase* getDatabase(const DbOrder& dbIndexFilesList, const Mernel::PropertyTree& customSegmentData) const noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
