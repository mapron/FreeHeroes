/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapScore.hpp"
#include "FHPos.hpp"

#include <vector>
#include <string>
#include <memory>
#include <set>

namespace FreeHeroes {

struct IZoneObject {
    virtual ~IZoneObject() = default;

    enum class Type
    {
        Visitable,
        Pickable,
        Joinable,  // monster join
        Removable, // Prison, lamp
    };

    using Mask = std::set<FHPos>;

    virtual void           place(FHPos pos) const     = 0;
    virtual Core::MapScore getScore() const           = 0;
    virtual void           setAccepted(bool accepted) = 0;
    virtual std::string    getId() const              = 0;
    virtual int64_t        getGuard() const           = 0;
    virtual Type           getType() const            = 0;
    virtual std::string    getRepulseId() const { return {}; }

    virtual Mask getVisitableMask() const { return { FHPos(0, 0) }; }
    virtual Mask getBlockedUnvisitableMask() const { return {}; }

    virtual bool preventDuplicates() const { return false; }
};

using IZoneObjectPtr = std::shared_ptr<IZoneObject>;

struct IZoneObjectGroup : public IZoneObject {
    virtual bool tryPush(const IZoneObjectPtr& item) = 0;
    virtual void updateMask()                        = 0;
};

using IZoneObjectGroupPtr = std::shared_ptr<IZoneObjectGroup>;

IZoneObjectGroupPtr makeNewZoneObjectGroup(int64_t maxGuard, size_t itemLimit, uint8_t rngMask);

using ZoneObjectList = std::vector<IZoneObjectPtr>;

struct ZoneObjectGeneration {
    ZoneObjectList m_segmentsGuarded;
    ZoneObjectList m_segmentsUnguardedNonPickables;
    ZoneObjectList m_segmentsUnguardedPickables;

    ZoneObjectList m_roadUnguardedPickables;

    std::vector<std::string> m_allIds; // for checking
};

}
