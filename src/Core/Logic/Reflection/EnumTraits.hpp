#pragma once

#include <tuple>

#include <frozen/unordered_map.h>
#include <frozen/string.h>

namespace FreeHeroes::Core::Reflection {

template<class T>
concept IsEnum = std::is_enum_v<T>;

struct EnumTraits {
    template<typename KeyType>
    static constexpr std::tuple<> linearTupleToPaired(const std::tuple<>&)
    {
        return {};
    }

    template<typename KeyType, typename... Ts>
    static constexpr auto linearTupleToPaired(const std::tuple<Ts...>& tuple)
    {
        // tuple<pair<>>!
        auto tupleHead      = std::make_tuple(std::make_pair(KeyType(std::get<0>(tuple)), std::get<1>(tuple)));
        auto tupleRemainder = std::apply([](const auto&, const auto&, const auto&... args) { return std::make_tuple(args...); }, tuple);
        return std::tuple_cat(tupleHead, linearTupleToPaired<KeyType>(tupleRemainder));
    }

    template<IsEnum Enum>
    using MappingStringToEnum = std::pair<frozen::string, Enum>;

    template<IsEnum Enum>
    using MappingEnumToString = std::pair<Enum, frozen::string>;

    template<IsEnum Enum>
    static constexpr MappingEnumToString<Enum> pairSwap(const MappingStringToEnum<Enum>& pair)
    {
        return std::make_pair(pair.second, pair.first);
    }

    template<IsEnum Enum, size_t size>
    struct Meta {
        Enum                                              m_default;
        frozen::unordered_map<frozen::string, Enum, size> m_toEnum;
        frozen::unordered_map<Enum, frozen::string, size> m_fromEnum;
    };

    template<IsEnum Enum, class... Types>
    constexpr static auto make(Enum defaultValue, Types&&... argPairs)
    {
        const auto enumKeyValuePairs = linearTupleToPaired<frozen::string>(std::tuple<Types...>(std::forward<Types>(argPairs)...));

        constexpr auto toArrayDirect  = [](auto&&... x) { return std::array{ std::forward<decltype(x)>(x)... }; };
        constexpr auto toArraySwapped = [](auto&&... x) { return std::array{ std::forward<decltype(pairSwap(x))>(pairSwap(x))... }; };

        const auto stringToEnumArray = std::apply(toArrayDirect, enumKeyValuePairs);
        const auto enumToStringArray = std::apply(toArraySwapped, enumKeyValuePairs);

        return Meta<Enum, sizeof...(Types) / 2>{
            defaultValue,
            frozen::make_unordered_map(stringToEnumArray),
            frozen::make_unordered_map(enumToStringArray)
        };
    }

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
/*
#define ENUM_ARG(ENUM_NAME, VAL) \
#VAL, ENUM_NAME::VAL

//#define ENUM_ARG_1P(ENUM_NAME, VAL, ...) \
//ENUM_ARG(ENUM_NAME, VAL) , ENUM_ARG_1P(ENUM_NAME, __VA_ARGS__)

#define REG_3(ENUM_NAME, DEF_NAME, VAL_1, VAL_2, VAL_3) \
    template<> \
    inline constexpr const auto EnumTraits::s_valueMapping<ENUM_NAME> = EnumTraits::make( \
        ENUM_NAME::DEF_NAME, \
        ENUM_ARG(ENUM_NAME, VAL_1), \
        ENUM_ARG(ENUM_NAME, VAL_2), \
        ENUM_ARG(ENUM_NAME, VAL_3));

REG_3(Enumchik, foo, foo, bar, baz)
*/
