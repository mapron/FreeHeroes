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

namespace FreeHeroes::Core {

class BonusRatio {
    int64_t m_num = 0;
    int64_t m_denom = 0;
public:
    BonusRatio() = default;
    bool isValid() const { return m_denom != 0;}
    BonusRatio(int64_t n, int64_t d) : m_num(n), m_denom(d) { simplifyRatio(); }

    // Convenience for script engine:
    void set(int64_t n, int64_t d) { m_num = n; m_denom = d; simplifyRatio(); }
    void add(int64_t n, int64_t d) { *this += BonusRatio(n, d); }
    void mult(int64_t n, int64_t d) { *this *= BonusRatio(n, d); }

    int64_t num() const noexcept { return m_num;}
    int64_t denom() const noexcept { return m_denom;}

    int64_t commonNumWith(const BonusRatio & other) const noexcept {
        auto lcm = std::lcm(m_denom, other.m_denom);
        return  m_num * (lcm / m_denom);
    }

    BonusRatio operator * (const int64_t & value) const noexcept {
        BonusRatio ret(m_num * value, m_denom);
        ret.simplifyRatio();
        return ret;
    }
    BonusRatio operator * (const BonusRatio & left) const noexcept {
        BonusRatio ret(m_num * left.m_num, m_denom * left.m_denom);
        ret.simplifyRatio();
        return ret;
    }
    BonusRatio& operator *= (const int64_t & value) noexcept {
        m_num *= value;
        simplifyRatio();
        return *this;
    }
    BonusRatio& operator *= (const BonusRatio & left) noexcept {
        m_num   *= left.m_num;
        m_denom *= left.m_denom;
        simplifyRatio();
        return *this;
    }

    BonusRatio operator / (const int64_t & value) const noexcept {
        BonusRatio ret(m_num, m_denom * value);
        ret.simplifyRatio();
        return ret;
    }
    BonusRatio operator / (const BonusRatio & left) const noexcept {
        BonusRatio ret(m_num * left.m_denom, m_denom * left.m_num);
        ret.simplifyRatio();
        return ret;
    }
    BonusRatio& operator /= (const int64_t & value) noexcept {
        m_denom *= value;
        simplifyRatio();
        return *this;
    }
    BonusRatio& operator /= (const BonusRatio & left) noexcept {
        m_num   *= left.m_denom;
        m_denom *= left.m_num;
        simplifyRatio();
        return *this;
    }

    BonusRatio operator - () const noexcept { return BonusRatio(-m_num, m_denom);}

    BonusRatio operator + (const BonusRatio & left) const noexcept {
        auto lcm = std::lcm(m_denom, left.m_denom);
        BonusRatio result(commonNumWith(left) + left.commonNumWith(*this), lcm);
        result.simplifyRatio();
        return result;
    }
    BonusRatio& operator += (const BonusRatio & left) noexcept {
        auto lcm = std::lcm(m_denom, left.m_denom);
        BonusRatio result(commonNumWith(left) + left.commonNumWith(*this), lcm);
        this->m_num = result.m_num;
        this->m_denom = result.m_denom;
        simplifyRatio();
        return *this;
    }

    BonusRatio operator - (const BonusRatio & left) const noexcept {
        return *this + (-left);
    }
    BonusRatio operator -= (const BonusRatio & left) noexcept {
        *this += (-left);
        return *this;
    }

    bool operator > (const BonusRatio & left) const noexcept {
        return commonNumWith(left) > left.commonNumWith(*this);
    }
    bool operator >= (const BonusRatio & left) const noexcept {
        return commonNumWith(left) >= left.commonNumWith(*this);
    }
    bool operator < (const BonusRatio & left) const noexcept {
        return commonNumWith(left) < left.commonNumWith(*this);
    }
    bool operator <= (const BonusRatio & left) const noexcept {
        return commonNumWith(left) <= left.commonNumWith(*this);
    }
    auto asTuple() const noexcept { return std::tie(m_num, m_denom); }
    bool operator == (const BonusRatio & left) const noexcept {
        return this->asTuple() == left.asTuple();
    }
    bool operator != (const BonusRatio & left) const noexcept {
        return this->asTuple() != left.asTuple();
    }

    void simplifyRatio() {
        int64_t gcd = std::gcd(m_num, m_denom);
        m_num   /= gcd;
        m_denom /= gcd;
        if (m_num < 0 && m_denom < 0)
        {
            m_num   = -m_num;
            m_denom = -m_denom;
        }
    }

    int64_t roundDown()          const noexcept { return m_num / m_denom; }
    int roundDownInt()           const noexcept { return static_cast<int>(roundDown()); }


    static int64_t calcAddIncrease(int64_t value, BonusRatio increase) noexcept {
        const BonusRatio valueRat(value, 1);
        return (valueRat + valueRat * increase).roundDown();
    }
    static int calcAddIncrease(int value, BonusRatio increase) noexcept {
        const BonusRatio valueRat(value, 1);
        return (valueRat + valueRat * increase).roundDownInt();
    }
    static int64_t calcSubDecrease(int64_t value, BonusRatio decrease, int64_t minimal = 0) noexcept {
        const BonusRatio valueRat(value, 1);
        value  = (valueRat - valueRat * decrease).roundDown();
        value = std::max(value, minimal);
        return value;
    }
    static int calcSubDecrease(int value, BonusRatio decrease, int minimal = 0) noexcept {
        const BonusRatio valueRat(value, 1);
        value  = (valueRat - valueRat * decrease).roundDownInt();
        value = std::max(value, minimal);
        return value;
    }
};

}
