/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MacroUtils.hpp"
#include "MetaInfo.hpp"

#define MAKE_FIELD_PAIR(typeBase, str, value) Field(str, &typeBase ::value),
#define MAKE_FIELD_SINGLE(typeBase, value) Field(#value, &typeBase ::value),

#define MAKE_FIELD_LIST_PAIRED(typeBase, ...) \
    INTERNAL_EXPAND(FOR_EACH_BY_PAIR(MAKE_FIELD_PAIR, typeBase, __VA_ARGS__))

#define MAKE_FIELD_LIST_STRINGIFY(typeBase, ...) \
    INTERNAL_EXPAND(FOR_EACH_BY_SINGLE(MAKE_FIELD_SINGLE, typeBase, __VA_ARGS__))

#define STRUCT_REFLECTION_PAIRED(typeBase, ...) \
    template<> \
    inline constexpr const std::tuple MetaInfo::s_fields<typeBase>{ \
        INTERNAL_EXPAND(MAKE_FIELD_LIST_PAIRED(typeBase, __VA_ARGS__)) \
    };

#define STRUCT_REFLECTION_STRINGIFY(typeBase, ...) \
    template<> \
    inline constexpr const std::tuple MetaInfo::s_fields<typeBase>{ \
        INTERNAL_EXPAND(MAKE_FIELD_LIST_STRINGIFY(typeBase, __VA_ARGS__)) \
    };
