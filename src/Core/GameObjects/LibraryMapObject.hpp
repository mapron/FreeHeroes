/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "LibraryResource.hpp"
#include "LibraryArtifact.hpp"

#include <string>
#include <vector>

namespace FreeHeroes::Core {

// clang-format off
enum class FieldLayout { Standard, Object, Churchyard1, Churchyard2, Ruins, Spit };
enum class FieldLayoutPos { None = -1, TR = 0, R, BR, BL, L, TL, T, B };
// clang-format on

struct UnitWithCount {
    LibraryUnitConstPtr unit  = nullptr;
    int                 count = 0;
};

struct LibraryMapObject {
    std::string id;

    std::string untranslatedName;
    FieldLayout fieldLayout = FieldLayout::Object;

    struct Reward {
        ResourceAmount       resources;
        UnitWithCount        unit;
        ArtifactRewardAmount artifacts;

        size_t totalItems() const noexcept { return resources.nonEmptyAmount() + (unit.unit != nullptr) + artifacts.totalAmount(); }
    };
    using Rewards = std::vector<Reward>;
    Rewards rewards;

    using Guards = std::vector<UnitWithCount>;

    struct Variant {
        std::string name;
        int         rewardIndex = 0;
        Guards      guards;
    };
    using Variants = std::vector<Variant>;
    Variants variants;

    struct Presentation {
        int         order = 0;
        std::string icon;
    };
    Presentation presentationParams;
};

}
