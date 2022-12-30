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
        setMaximumSize(m_internal.size());
        setSize(m_internal.size());
        reset();
    }

    ByteArrayHolder&       getHolder() { return m_internal; }
    const ByteArrayHolder& getHolder() const { return m_internal; }

    /// Pointers to begin and end bytes, just as in STL. If ByteOrderBuffer is empty, begin and end will be nullptr.
    inline uint8_t*       begin() { return m_beg; }
    inline const uint8_t* begin() const { return m_beg; }
    inline uint8_t*       end() { return m_beg + m_size; }
    inline const uint8_t* end() const { return m_beg + m_size; }

    /// return pointer to current read position, ensuring that buffer have at least required size after that. Otherwise, returns null.
    inline const uint8_t* posRead(size_t required = 0)
    {
        if (m_posRead + static_cast<ptrdiff_t>(required) > m_size) {
            m_eofRead = true;
            throw std::runtime_error("EOF read buffer reached on offset: " + std::to_string(getOffsetRead())
                                     + ", requested " + std::to_string(required) + " more bytes, which is beyond size=" + std::to_string(getSize()));
        }
        return m_beg + m_posRead;
    }
    /// return pointer to current write position, ensuring that buffer have required bytes after that. If possible, buffer grows.
    inline uint8_t* posWrite(size_t required = 0)
    {
        ptrdiff_t r = getRemainWrite();
        if (r < ptrdiff_t(required) && !setSize(getSize() + required - r))
            throw std::runtime_error("EOF write buffer reached on offset: " + std::to_string(getOffsetWrite() + required));

        return m_beg + m_posWrite;
    }

    /// Current read/write positions counting from begin and end.
    inline ptrdiff_t getOffsetRead() const { return m_posRead; }
    inline ptrdiff_t getOffsetWrite() const { return m_posWrite; }
    inline ptrdiff_t getRemainRead() const { return m_size - m_posRead; }
    inline ptrdiff_t getRemainWrite() const { return m_size - m_posWrite; }

    /// Size of logical buffer (may be less than actual storage).
    inline size_t getSize() const { return static_cast<size_t>(m_size); }

    /// Try to resize buffer. If resize fails, return false.
    bool setSize(size_t sz)
    {
        if (!m_resizeEnabled)
            return false;

        if (sz != getSize())
            setMaximumSize(sz);

        m_size = sz;
        if (getRemainRead() < 0)
            resetRead();
        if (getRemainWrite() < 0)
            resetWrite();

        return true;
    }
    void clear() { setSize(0); }

    /// Functions operating on buffer positions.
    inline void setOffsetRead(ptrdiff_t offset) { m_posRead = offset; }
    inline void setOffsetWrite(ptrdiff_t offset) { m_posWrite = offset; }

    inline void extendWriteToCurrentOffset()
    {
        if (getRemainWrite() <= 0)
            setSize(m_posWrite + 1);
    }

    inline void resetRead()
    {
        m_posRead = 0;
        m_eofRead = false;
    }
    inline void resetWrite()
    {
        m_posWrite = 0;
        m_eofWrite = false;
    }
    inline void reset()
    {
        resetRead();
        resetWrite();
    }

    inline void markRead(ptrdiff_t size) { m_posRead += size; }
    inline void markWrite(ptrdiff_t size) { m_posWrite += size; }

    inline bool eofRead() const { return m_eofRead; }
    inline bool eofWrite() const { return m_eofWrite; }

    /// like a PosRead function, but returns true if space enough.
    bool checkRemain(size_t minimum) const
    {
        if (getRemainRead() < ptrdiff_t(minimum))
            return false;

        return !m_eofRead;
    }

    /// Removes sz bytes from buffer begin.
    bool removeFromStart(size_t sz)
    {
        if (!sz)
            return false;
        if (sz >= getSize())
            return setSize(0);

        removeFromStartInternal(sz);

        return true;
    }

    void setResizeEnabled(bool state)
    {
        m_resizeEnabled = state;
    }

    /// debugging functions.
    static inline char toHex(uint8_t c)
    {
        return (c <= 9) ? '0' + c : 'a' + c - 10;
    }

    /// Debug output.
    std::string toHex(bool insertSpaces = true, bool insertPos = false) const
    {
        std::string ret;
        if (!m_beg || !getSize())
            return ret;
        for (uint8_t* p = m_beg; p < end(); p++) {
            uint8_t l = *p % 16;
            uint8_t h = *p / 16;
            ret += toHex(h);
            ret += toHex(l);
            if (insertSpaces)
                ret += ' ';
            if (insertPos) {
                if (p == m_beg + m_posRead)
                    ret += "|<R| ";
                if (p == m_beg + m_posWrite)
                    ret += "|<W| ";
            }
        }
        if (insertPos) {
            if (m_size == m_posRead)
                ret += "|<R| ";
            if (m_size == m_posWrite)
                ret += "|<W| ";
            if (m_eofRead)
                ret += "[eofRead] ";
            if (m_eofWrite)
                ret += "[eofWrite] ";
        }
        return ret;
    }

    std::string getDebugInfo() const
    {
        std::ostringstream os;
        os << "beg=" << intptr_t(m_beg)
           << ", size=" << getSize()
           << ", r=" << intptr_t(m_posRead) << " [" << getOffsetRead() << "]"
           << ", w=" << intptr_t(m_posWrite) << " [" << getOffsetWrite() << "]";
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

    void setMaximumSize(size_t maxSize)
    {
        ptrdiff_t oRead  = getOffsetRead();
        ptrdiff_t oWrite = getOffsetWrite();
        ptrdiff_t oSize  = getSize();

        m_internal.resize(maxSize);
        if (maxSize) {
            m_beg  = m_internal.data();
            m_size = oSize;
        } else {
            m_beg  = nullptr;
            m_size = 0;
        }
        setOffsetRead(oRead);
        setOffsetWrite(oWrite);
    }
    void removeFromStartInternal(size_t rem)
    {
        ptrdiff_t oRead  = getOffsetRead() - rem;
        ptrdiff_t oWrite = getOffsetWrite() - rem;
        ptrdiff_t oSize  = getSize() - rem;
        if (oRead < 0)
            oRead = 0;
        if (oWrite < 0)
            oWrite = 0;

        m_internal.ref().erase(m_internal.ref().begin(), m_internal.ref().begin() + rem);
        m_beg  = m_internal.data();
        m_size = oSize;
        setOffsetRead(oRead);
        setOffsetWrite(oWrite);
    }

private:
    ByteArrayHolder m_internal;
    ptrdiff_t       m_posRead  = 0;
    ptrdiff_t       m_posWrite = 0;
    uint8_t*        m_beg      = nullptr;
    ptrdiff_t       m_size     = 0;

    bool m_eofRead       = false;
    bool m_eofWrite      = false;
    bool m_resizeEnabled = true;
};

}
