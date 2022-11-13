/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once
#include "ByteBuffer.hpp"

#include <stddef.h>
#include <cstring>
#include <sstream>

namespace FreeHeroes {

/// Class wraps some blob data to use in read/write operations in ByteOrderStream.
/// Uses ByteArrayHolder as internal storage
class ByteOrderBuffer {
public:
    ByteOrderBuffer(const ByteArrayHolder& holder = ByteArrayHolder())
        : m_internal(holder)
    {
        SetMaximumSize(m_internal.size());
        SetSize(m_internal.size());
        Reset();
    }

    ByteArrayHolder&       GetHolder() { return m_internal; }
    const ByteArrayHolder& GetHolder() const { return m_internal; }

    /// Pointers to begin and end bytes, just as in STL. If ByteOrderBuffer is empty, begin and end will be nullptr.
    inline uint8_t*       begin() { return m_beg; }
    inline const uint8_t* begin() const { return m_beg; }
    inline uint8_t*       end() { return m_end; }
    inline const uint8_t* end() const { return m_end; }

    /// return pointer to current read position, ensuring that buffer have at least required size after that. Otherwise, returns null.
    inline const uint8_t* PosRead(size_t required = 0)
    {
        if (m_posRead + required > m_end) {
            m_eofRead = true;
            throw std::runtime_error("EOF read buffer reached on offset: " + std::to_string(GetOffsetRead())
                                     + ", requested " + std::to_string(required) + " more bytes, which is beyond size=" + std::to_string(GetSize()));
        }
        return m_posRead;
    }
    /// return pointer to current write position, ensuring that buffer have required bytes after that. If possible, buffer grows.
    inline uint8_t* PosWrite(size_t required = 0)
    {
        ptrdiff_t r = GetRemainWrite();
        if (r < ptrdiff_t(required) && !SetSize(GetSize() + required - r))
            throw std::runtime_error("EOF write buffer reached on offset: " + std::to_string(GetOffsetWrite() + required));

        return m_posWrite;
    }

    /// Current read/write positions counting from begin and end.
    inline ptrdiff_t GetOffsetRead() const { return m_posRead - m_beg; }
    inline ptrdiff_t GetOffsetWrite() const { return m_posWrite - m_beg; }
    inline ptrdiff_t GetRemainRead() const { return m_end - m_posRead; }
    inline ptrdiff_t GetRemainWrite() const { return m_end - m_posWrite; }

    /// Size of logical buffer (may be less than actual storage).
    inline size_t GetSize() const { return m_end - m_beg; }

    /// Try to resize buffer. If resize fails, return false.
    bool SetSize(size_t sz)
    {
        if (sz != GetSize())
            SetMaximumSize(sz);

        m_end = m_beg + sz;
        if (GetRemainRead() < 0)
            ResetRead();
        if (GetRemainWrite() < 0)
            ResetWrite();

        return true;
    }
    void Clear() { SetSize(0); }

    /// Functions operating on buffer positions.
    inline void SetOffsetRead(ptrdiff_t offset) { m_posRead = m_beg + offset; }
    inline void SetOffsetWrite(ptrdiff_t offset) { m_posWrite = m_beg + offset; }

    inline void ResetRead()
    {
        m_posRead = m_beg;
        m_eofRead = false;
    }
    inline void ResetWrite()
    {
        m_posWrite = m_beg;
        m_eofWrite = false;
    }
    inline void Reset()
    {
        ResetRead();
        ResetWrite();
    }

    inline void MarkRead(ptrdiff_t size) { m_posRead += size; }
    inline void MarkWrite(ptrdiff_t size) { m_posWrite += size; }

    inline bool EofRead() const { return m_eofRead; }
    inline bool EofWrite() const { return m_eofWrite; }

    /// like a PosRead function, but returns true if space enough.
    bool CheckRemain(size_t minimum) const
    {
        if (GetRemainRead() < ptrdiff_t(minimum))
            return false;

        return !m_eofRead;
    }

    /// Removes sz bytes from buffer begin.
    bool RemoveFromStart(size_t sz)
    {
        if (!sz)
            return false;
        if (sz >= GetSize())
            return SetSize(0);

        RemoveFromStartInternal(sz);

        return true;
    }

    /// debugging functions.
    static inline char ToHex(uint8_t c)
    {
        return (c <= 9) ? '0' + c : 'a' + c - 10;
    }

    /// Debug output.
    std::string ToHex(bool insertSpaces = true, bool insertPos = false) const
    {
        std::string ret;
        if (!m_beg || !GetSize())
            return ret;
        for (uint8_t* p = m_beg; p < m_end; p++) {
            uint8_t l = *p % 16;
            uint8_t h = *p / 16;
            ret += ToHex(h);
            ret += ToHex(l);
            if (insertSpaces)
                ret += ' ';
            if (insertPos) {
                if (p == m_posRead)
                    ret += "|<R| ";
                if (p == m_posWrite)
                    ret += "|<W| ";
            }
        }
        if (insertPos) {
            if (m_end == m_posRead)
                ret += "|<R| ";
            if (m_end == m_posWrite)
                ret += "|<W| ";
            if (m_eofRead)
                ret += "[eofRead] ";
            if (m_eofWrite)
                ret += "[eofWrite] ";
        }
        return ret;
    }

    std::string GetDebugInfo() const
    {
        std::ostringstream os;
        os << "beg=" << intptr_t(m_beg)
           << ", end=" << intptr_t(m_end)
           << ", size=" << GetSize()
           << ", r=" << intptr_t(m_posRead) << " [" << GetOffsetRead() << "]"
           << ", w=" << intptr_t(m_posWrite) << " [" << GetOffsetWrite() << "]";
        if (m_eofRead)
            os << "[eofRead] ";
        if (m_eofWrite)
            os << "[eofWrite] ";
        os << ", internal=" << intptr_t(m_internal.data()) << ", size=" << m_internal.size();
        return os.str();
    }

private:
    ByteOrderBuffer(const ByteOrderBuffer& another) = delete;
    ByteOrderBuffer(ByteOrderBuffer&& another)      = delete;

    ByteOrderBuffer& operator=(const ByteOrderBuffer& another) = delete;
    ByteOrderBuffer& operator=(ByteOrderBuffer&& another) = delete;

    void SetMaximumSize(size_t maxSize)
    {
        ptrdiff_t oRead  = GetOffsetRead();
        ptrdiff_t oWrite = GetOffsetWrite();
        ptrdiff_t oSize  = GetSize();

        m_internal.resize(maxSize);
        if (maxSize) {
            m_beg = m_internal.data();
            m_end = m_beg + oSize;
        } else {
            m_beg = m_end = nullptr;
        }
        SetOffsetRead(oRead);
        SetOffsetWrite(oWrite);
    }
    void RemoveFromStartInternal(size_t rem)
    {
        ptrdiff_t oRead  = GetOffsetRead() - rem;
        ptrdiff_t oWrite = GetOffsetWrite() - rem;
        ptrdiff_t oSize  = GetSize() - rem;
        if (oRead < 0)
            oRead = 0;
        if (oWrite < 0)
            oWrite = 0;

        m_internal.ref().erase(m_internal.ref().begin(), m_internal.ref().begin() + rem);
        m_beg = m_internal.data();
        m_end = m_beg + oSize;
        SetOffsetRead(oRead);
        SetOffsetWrite(oWrite);
    }

private:
    ByteArrayHolder m_internal;
    uint8_t*        m_posRead  = nullptr;
    uint8_t*        m_posWrite = nullptr;
    uint8_t*        m_beg      = nullptr;
    uint8_t*        m_end      = nullptr;

    bool m_eofRead  = false;
    bool m_eofWrite = false;
};

}
