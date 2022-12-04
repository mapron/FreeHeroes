/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "Reflection/PropertyTreeWriter.hpp"

#include <gtest/gtest.h>

struct SomeClass {
    int         m_one = 13;
    bool        m_bar = true;
    std::string m_baz = "baz";
};
struct WrapperClass {
    SomeClass m_f{ .m_one = 42 };
};

using namespace FreeHeroes;
using namespace Core;

namespace FreeHeroes::Core::Reflection {

template<>
inline constexpr const std::tuple MetaInfo::s_fields<SomeClass>{
    Field("one", &SomeClass::m_one),
    Field("bar", &SomeClass::m_bar),
    Field("baz", &SomeClass::m_baz),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<WrapperClass>{

    Field("f", &WrapperClass::m_f),
};

}

GTEST_TEST(Serialize, Basic)
{
    SomeClass                      inst;
    PropertyTree                   jsonActual;
    Reflection::PropertyTreeWriter writer;
    writer.valueToJson(inst, jsonActual);

    PropertyTree jsonExpected;
    jsonExpected.convertToMap();
    EXPECT_EQ(jsonExpected, jsonActual);

    inst.m_bar          = false;
    jsonExpected["bar"] = PropertyTreeScalar(false);
    writer.valueToJson(inst, jsonActual);
    EXPECT_EQ(jsonExpected, jsonActual);
}

GTEST_TEST(Serialize, Wrapped)
{
    WrapperClass                   wrapper;
    PropertyTree                   jsonActual;
    Reflection::PropertyTreeWriter writer;
    writer.valueToJson(wrapper, jsonActual);

    PropertyTree jsonExpected;
    jsonExpected.convertToMap();
    jsonExpected["f"]["one"] = PropertyTreeScalar(42);
    EXPECT_EQ(jsonExpected, jsonActual);

    wrapper.m_f.m_baz        = "foo";
    jsonExpected["f"]["baz"] = PropertyTreeScalar("foo");
    writer.valueToJson(wrapper, jsonActual);
    EXPECT_EQ(jsonExpected, jsonActual);
}
