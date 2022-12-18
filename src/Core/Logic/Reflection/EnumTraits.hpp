/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <tuple>

#include <frozen/unordered_map.h>
#include <frozen/string.h>

namespace FreeHeroes::Core::Reflection {

template<class T>
concept IsEnum = std::is_enum_v<T>;

struct EnumTraits {
    template<IsEnum Enum, size_t size>
    struct Meta {
        Enum                                              m_default;
        frozen::unordered_map<frozen::string, Enum, size> m_toEnum;
        frozen::unordered_map<Enum, frozen::string, size> m_fromEnum;
    };

    template<IsEnum Enum>
    static inline constexpr const bool s_valueMapping{ false };

    template<IsEnum Enum>
    constexpr static frozen::string enumToString(Enum value)
    {
        const auto&    mappingInfo = s_valueMapping<Enum>;
        frozen::string defValue    = "";
        const auto&    mapping     = mappingInfo.m_fromEnum;
        auto           it          = mapping.find(value);
        return it != mapping.cend() ? it->second : defValue;
    }

    template<IsEnum Enum>
    constexpr static Enum stringToEnum(frozen::string str)
    {
        const auto& mappingInfo = s_valueMapping<Enum>;
        Enum        defValue    = mappingInfo.m_default;
        const auto& mapping     = mappingInfo.m_toEnum;
        auto        it          = mapping.find(str);
        return it != mapping.cend() ? it->second : defValue;
    }
};
}
