/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ZoneObject.hpp"

#include "FHMapObject.hpp"

#include <sstream>
#include <cassert>

namespace FreeHeroes {

namespace {

class ZoneObjectGroup : public IZoneObjectGroup {
public:
    struct Item {
        IZoneObjectPtr m_obj;
        FHPos          m_relPos;
    };
    std::vector<Item> m_items;

    using Type = IZoneObject::Type;

    IZoneObject::Type m_type = IZoneObject::Type::Visitable;

    int64_t        m_maxGuard = 0;
    int64_t        m_guard    = 0;
    std::string    m_repulseId;
    size_t         m_itemLimit = 0;
    uint8_t        m_rngMask   = 0;
    Core::MapScore m_score;

    Mask        m_mask;
    std::string m_id;

    ZoneObjectGroup() = default;
    ZoneObjectGroup(int64_t maxGuard, size_t itemLimit, uint8_t rngMask)
        : m_maxGuard(maxGuard)
        , m_itemLimit(itemLimit)
        , m_rngMask(rngMask)
    {
    }

    void           place(FHPos pos) const override;
    Core::MapScore getScore() const override
    {
        return m_score;
    }
    void setAccepted(bool accepted) override
    {
        assert(!"Liskov unhappy but it shouldn't be called");
    }
    std::string getId() const override { return m_id; }
    int64_t     getGuard() const override { return m_guard; }
    Type        getType() const override { return Type::Pickable; }
    std::string getRepulseId() const override { return m_repulseId; }

    Mask getVisitableMask() const override { return m_mask; }

    bool tryPush(const IZoneObjectPtr& item) override;
    void updateMask() override;
};

}

void ZoneObjectGroup::place(FHPos pos) const
{
    for (auto& item : m_items) {
        item.m_obj->place(pos + item.m_relPos);
    }
}

bool ZoneObjectGroup::tryPush(const IZoneObjectPtr& item)
{
    if (item->getType() != Type::Pickable)
        return false;
    if (m_items.size() >= m_itemLimit)
        return false;

    int64_t newGuard = m_guard + item->getGuard();

    if (!m_items.empty()) {
        if (newGuard > m_maxGuard)
            return false;
    }
    if (item->preventDuplicates()) {
        for (auto& existingItem : m_items) {
            if (existingItem.m_obj->getId() == item->getId())
                return false;
        }
    }
    if (!m_repulseId.empty() && !item->getRepulseId().empty())
        return false;
    m_guard     = newGuard;
    m_score     = m_score + item->getScore();
    m_repulseId = item->getRepulseId();
    if (!m_id.empty())
        m_id += "+";
    m_id += item->getId();

    m_items.push_back(Item{ .m_obj = item });
    return true;
}

void ZoneObjectGroup::updateMask()
{
    m_mask.clear();
    size_t                        itemCount      = m_items.size();
    const size_t                  maxRowSize     = itemCount >= 6 ? 3 : 2;
    [[maybe_unused]] const size_t itemRectHeight = (itemCount + maxRowSize - 1) / maxRowSize;
    const size_t                  itemRectWidth  = std::min(maxRowSize, itemCount);

    for (size_t index = 0; auto& item : m_items) {
        FHPos itemOffset;

        // @todo: utilize m_rng for variety
        itemOffset.m_y -= index / itemRectWidth;
        itemOffset.m_x -= index % itemRectWidth;

        m_mask.insert(itemOffset);
        auto itemMask = item.m_obj->getVisitableMask();
        assert(itemMask.size() == 1);
        auto childVisitableOffset = *itemMask.begin(); // (-1, 0) for lamp, for example.
        item.m_relPos             = itemOffset - childVisitableOffset;

        index++;
    }
}

IZoneObjectGroupPtr makeNewZoneObjectGroup(int64_t maxGuard, size_t itemLimit, uint8_t rngMask)
{
    return std::make_shared<ZoneObjectGroup>(maxGuard, itemLimit, rngMask);
}

}
