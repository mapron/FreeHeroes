/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IResourceLibrary.hpp"

#include "CoreResourceExport.hpp"

namespace FreeHeroes::Core {

class CORERESOURCE_EXPORT ResourceLibraryFactory : public IResourceLibraryFactory {
public:
    ResourceLibraryFactory();
    ~ResourceLibraryFactory();

    void scanForMods(const Mernel::std_path& root);

    void scanModSubfolders() const noexcept;

    IResourceLibrary::ConstPtr create(const ModOrder& loadOrder) const noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
