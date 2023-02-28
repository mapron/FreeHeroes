/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <tuple>
#include <type_traits>
#include <cstddef>

namespace FreeHeroes {

template<class Tuple>
class Resolver {
public:
    static const constexpr auto s_tupleSize = std::tuple_size_v<Tuple>;

    Resolver(const Tuple& tuple)
        : m_tuple(tuple)
    {}

    template<size_t index, class T, class... Types>
    T* createFromTupleElement(Types&&... args) const
    {
        if constexpr (std::is_constructible_v<T, std::tuple_element_t<index, Tuple>, Types...>)
            return new T(std::get<index>(m_tuple), std::forward<Types>(args)...);
        else if constexpr (s_tupleSize > index + 1)
            return createFromTupleElement<index + 1, T>(std::forward<Types>(args)...);
        else
            return new T(std::forward<Types>(args)...);
    }

    template<class T, class... Types>
    T* create(Types&&... args) const
    {
        if constexpr (std::is_constructible_v<T, Tuple, Types...>)
            return new T(m_tuple, std::forward<Types>(args)...);
        else if constexpr (s_tupleSize > 0)
            return createFromTupleElement<0, T>(std::forward<Types>(args)...);
        else
            return new T(std::forward<Types>(args)...);
    }

private:
    const Tuple& m_tuple;
};

}
