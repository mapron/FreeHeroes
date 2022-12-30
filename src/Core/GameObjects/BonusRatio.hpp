/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <numeric>
#include <tuple>
#include <cstdint>
#include <algorithm>

#include <string>

namespace FreeHeroes::Core {

class BonusRatio {
public:
    BonusRatio() = default;
    constexpr BonusRatio(int64_t n, int64_t d)
        : m_num(n)
        , m_denom(d)
    {
        simplifyRatio();
    }

    void fromString(std::string value)
    {
        *this          = {};
        const auto pos = value.find('/');
        if (pos == std::string::npos)
            return;
        value[pos] = 0;
        int n      = std::atoi(value.c_str());
        int d      = std::atoi(value.c_str() + pos + 1);
        if (d != 0)
            *this = { n, d };
    }

    constexpr bool isValid() const { return m_denom != 0; }

    constexpr int64_t num() const noexcept { return m_num; }
    constexpr int64_t denom() const noexcept { return m_denom; }

    constexpr int64_t commonNumWith(const BonusRatio& other) const noexcept
    {
        auto lcm = std::lcm(m_denom, other.m_denom);
        return m_num * (lcm / m_denom);
    }

    constexpr BonusRatio operator*(const int64_t& value) const noexcept
    {
        BonusRatio ret(m_num * value, m_denom);
        ret.simplifyRatio();
        return ret;
    }
    constexpr BonusRatio operator*(const BonusRatio& left) const noexcept
    {
        BonusRatio ret(m_num * left.m_num, m_denom * left.m_denom);
        ret.simplifyRatio();
        return ret;
    }
    constexpr BonusRatio& operator*=(const int64_t& value) noexcept
    {
        m_num *= value;
        simplifyRatio();
        return *this;
    }
    constexpr BonusRatio& operator*=(const BonusRatio& left) noexcept
    {
        m_num *= left.m_num;
        m_denom *= left.m_denom;
        simplifyRatio();
        return *this;
    }

    constexpr BonusRatio operator/(const int64_t& value) const noexcept
    {
        BonusRatio ret(m_num, m_denom * value);
        ret.simplifyRatio();
        return ret;
    }
    constexpr BonusRatio operator/(const BonusRatio& left) const noexcept
    {
        BonusRatio ret(m_num * left.m_denom, m_denom * left.m_num);
        ret.simplifyRatio();
        return ret;
    }
    constexpr BonusRatio& operator/=(const int64_t& value) noexcept
    {
        m_denom *= value;
        simplifyRatio();
        return *this;
    }
    constexpr BonusRatio& operator/=(const BonusRatio& left) noexcept
    {
        m_num *= left.m_denom;
        m_denom *= left.m_num;
        simplifyRatio();
        return *this;
    }

    constexpr BonusRatio operator-() const noexcept { return BonusRatio(-m_num, m_denom); }

    constexpr BonusRatio operator+(const BonusRatio& left) const noexcept
    {
        auto       lcm = std::lcm(m_denom, left.m_denom);
        BonusRatio result(commonNumWith(left) + left.commonNumWith(*this), lcm);
        result.simplifyRatio();
        return result;
    }
    constexpr BonusRatio& operator+=(const BonusRatio& left) noexcept
    {
        auto       lcm = std::lcm(m_denom, left.m_denom);
        BonusRatio result(commonNumWith(left) + left.commonNumWith(*this), lcm);
        this->m_num   = result.m_num;
        this->m_denom = result.m_denom;
        simplifyRatio();
        return *this;
    }

    constexpr BonusRatio operator-(const BonusRatio& left) const noexcept
    {
        return *this + (-left);
    }
    constexpr BonusRatio operator-=(const BonusRatio& left) noexcept
    {
        *this += (-left);
        return *this;
    }

    constexpr bool operator>(const BonusRatio& left) const noexcept
    {
        return commonNumWith(left) > left.commonNumWith(*this);
    }
    constexpr bool operator>=(const BonusRatio& left) const noexcept
    {
        return commonNumWith(left) >= left.commonNumWith(*this);
    }
    constexpr bool operator<(const BonusRatio& left) const noexcept
    {
        return commonNumWith(left) < left.commonNumWith(*this);
    }
    constexpr bool operator<=(const BonusRatio& left) const noexcept
    {
        return commonNumWith(left) <= left.commonNumWith(*this);
    }
    constexpr bool operator==(const BonusRatio& left) const noexcept
    {
        return commonNumWith(left) == left.commonNumWith(*this);
    }
    constexpr bool operator!=(const BonusRatio& left) const noexcept
    {
        return commonNumWith(left) != left.commonNumWith(*this);
    }

    constexpr void simplifyRatio()
    {
        int64_t gcd = std::gcd(m_num, m_denom);
        m_num /= gcd;
        m_denom /= gcd;
        if (m_num < 0 && m_denom < 0) {
            m_num   = -m_num;
            m_denom = -m_denom;
        }
    }

    constexpr int64_t roundDown() const noexcept { return m_num / m_denom; }
    constexpr int     roundDownInt() const noexcept { return static_cast<int>(roundDown()); }

    // Convenience for script engine:
    void set(int64_t n, int64_t d)
    {
        m_num   = n;
        m_denom = d;
        simplifyRatio();
    }
    void add(int64_t n, int64_t d) { *this += BonusRatio(n, d); }
    void mult(int64_t n, int64_t d) { *this *= BonusRatio(n, d); }

    static int64_t calcAddIncrease(int64_t value, BonusRatio increase) noexcept
    {
        const BonusRatio valueRat(value, 1);
        return (valueRat + valueRat * increase).roundDown();
    }
    static int calcAddIncrease(int value, BonusRatio increase) noexcept
    {
        const BonusRatio valueRat(value, 1);
        return (valueRat + valueRat * increase).roundDownInt();
    }
    static int64_t calcSubDecrease(int64_t value, BonusRatio decrease, int64_t minimal = 0) noexcept
    {
        const BonusRatio valueRat(value, 1);
        value = (valueRat - valueRat * decrease).roundDown();
        value = std::max(value, minimal);
        return value;
    }
    static int calcSubDecrease(int value, BonusRatio decrease, int minimal = 0) noexcept
    {
        const BonusRatio valueRat(value, 1);
        value = (valueRat - valueRat * decrease).roundDownInt();
        value = std::max(value, minimal);
        return value;
    }

    //private:
    int64_t m_num   = 0;
    int64_t m_denom = 0;
};

}
