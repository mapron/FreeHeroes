/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MacroUtils.hpp"
#include "EnumTraits.hpp"

#define MAKE_TO_ENUM_PAIR(enumBase, str, value) std::pair{ frozen::string(str), enumBase::value },
#define MAKE_FROM_ENUM_PAIR(enumBase, str, value) std::pair{ enumBase::value, frozen::string(str) },

#define MAKE_TO_ENUM_SINGLE(enumBase, value) std::pair{ frozen::string(#value), enumBase::value },
#define MAKE_FROM_ENUM_SINGLE(enumBase, value) std::pair{ enumBase::value, frozen::string(#value) },

#define MAKE_TO_FROM_ENUM_LIST_PAIRED(enumBase, ...) \
    .m_toEnum   = frozen::make_unordered_map({ INTERNAL_EXPAND(FOR_EACH_BY_PAIR(MAKE_TO_ENUM_PAIR, enumBase, __VA_ARGS__)) }), \
    .m_fromEnum = frozen::make_unordered_map({ INTERNAL_EXPAND(FOR_EACH_BY_PAIR(MAKE_FROM_ENUM_PAIR, enumBase, __VA_ARGS__)) }),

#define MAKE_TO_FROM_ENUM_LIST_SINGLE(enumBase, ...) \
    .m_toEnum   = frozen::make_unordered_map({ INTERNAL_EXPAND(FOR_EACH_BY_SINGLE(MAKE_TO_ENUM_SINGLE, enumBase, __VA_ARGS__)) }), \
    .m_fromEnum = frozen::make_unordered_map({ INTERNAL_EXPAND(FOR_EACH_BY_SINGLE(MAKE_FROM_ENUM_SINGLE, enumBase, __VA_ARGS__)) }),

#define ENUM_REFLECTION_PAIRED(enumBase, def, ...) \
    template<> \
    inline constexpr const auto EnumTraits::s_valueMapping<enumBase> = EnumTraits::Meta<enumBase, (GET_ARG_COUNT(__VA_ARGS__) / 2)>{ \
        .m_default = enumBase::def, \
        INTERNAL_EXPAND(MAKE_TO_FROM_ENUM_LIST_PAIRED(enumBase, __VA_ARGS__)) \
    };

#define ENUM_REFLECTION_STRINGIY(enumBase, def, ...) \
    template<> \
    inline constexpr const auto EnumTraits::s_valueMapping<enumBase> = EnumTraits::Meta<enumBase, GET_ARG_COUNT(__VA_ARGS__)>{ \
        .m_default = enumBase::def, \
        INTERNAL_EXPAND(MAKE_TO_FROM_ENUM_LIST_SINGLE(enumBase, __VA_ARGS__)) \
    };
