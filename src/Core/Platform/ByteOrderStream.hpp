/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ByteOrderStream_macro.hpp"
#include "ByteOrderBuffer.hpp"

#include <deque>
#include <map>
#include <type_traits>

namespace FreeHeroes {
class ByteOrderDataStreamReader;
class ByteOrderDataStreamWriter;
template<class T>
concept HasRead = requires(T t, ByteOrderDataStreamReader& reader)
{
    t.readBinary(reader);
};
template<class T>
concept HasWrite = requires(T t, ByteOrderDataStreamWriter& writer)
{
    t.writeBinary(writer);
};

inline constexpr uint_fast8_t createByteorderMask(uint_fast8_t endiannes8, uint_fast8_t endiannes16, uint_fast8_t endiannes32)
{
    return endiannes8 | (endiannes16 << 1) | (endiannes32 << 2);
}

/**
 * @brief Data stream which can have any byte order.
 *
 * Operates on ByteOrderBuffer. You can specify byte order for stream in constructor.
 *
 * General usage:
 *
 * ByteOrderBuffer buffer;
 * ByteOrderDataStreamWriter stream(buffer); // Creating stream;
 * stream << uint32_t(42);                   // Writing scalar data;
 * stream << std::string("Foo bar");         // Writing binary strings;
 *
 * buffer.begin(), buffer.size()             // retrieving serialized data.
 */
class ByteOrderDataStream {
public:
    /// Creates stream object. ProtocolMask sets actual stream byte order: for byte, words and dwords. Default is Big Endian for all.
    inline ByteOrderDataStream(ByteOrderBuffer& buf, uint_fast8_t ProtocolMask)
        : m_buf(buf)
    {
        setMask(ProtocolMask);
    }

    inline ByteOrderBuffer&       getBuffer() { return m_buf; }
    inline const ByteOrderBuffer& getBuffer() const { return m_buf; }

    /// Set byteorder settings for stream. To create actual value, use createByteorderMask.
    inline void setMask(uint_fast8_t protocolMask)
    {
        protocolMask &= 7;
        m_maskInt64  = (HOST_BYTE_ORDER_INT | HOST_WORD_ORDER_INT << 1 | HOST_DWORD_ORDER_INT64 << 2) ^ protocolMask;
        m_maskDouble = (HOST_BYTE_ORDER_FLOAT | HOST_WORD_ORDER_FLOAT << 1 | HOST_DWORD_ORDER_DOUBLE << 2) ^ protocolMask;
        m_maskInt32  = m_maskInt64 & 3;
        m_maskInt16  = m_maskInt32 & 1;
        m_maskInt8   = 0;
        m_maskFloat  = m_maskDouble & 3;
    }
    /// Creating mask description of byteorder. endiannes8 - byte order in word, endiannes16 - word order in dword, endiannes32 - dword order in qword.
    template<typename T>
    inline uint_fast8_t getTypeMask() const
    {
        return 0;
    }

    struct SizeGuard {
        ByteOrderDataStream* m_parent   = nullptr;
        uint_fast8_t         m_prevSize = 0;
        ~SizeGuard() { m_parent->m_containerSizeBytes = m_prevSize; }
    };

    SizeGuard setContainerSizeBytesGuarded(uint_fast8_t size)
    {
        auto prev            = m_containerSizeBytes;
        m_containerSizeBytes = size;
        return { this, prev };
    }

    static constexpr const uint_fast8_t BIG_ENDIAN    = createByteorderMask(ORDER_BE, ORDER_BE, ORDER_BE);
    static constexpr const uint_fast8_t LITTLE_ENDIAN = createByteorderMask(ORDER_LE, ORDER_LE, ORDER_LE);

protected:
    ByteOrderDataStream(const ByteOrderDataStream& another) = delete;
    ByteOrderDataStream(ByteOrderDataStream&& another)      = delete;

    ByteOrderDataStream& operator=(const ByteOrderDataStream& another) = delete;
    ByteOrderDataStream& operator=(ByteOrderDataStream&& another) = delete;

    ByteOrderBuffer& m_buf;
    uint_fast8_t     m_maskInt8;
    uint_fast8_t     m_maskInt16;
    uint_fast8_t     m_maskInt32;
    uint_fast8_t     m_maskInt64;
    uint_fast8_t     m_maskFloat;
    uint_fast8_t     m_maskDouble;
    uint_fast8_t     m_containerSizeBytes{ 4 };
};

class ByteOrderDataStreamReader : public ByteOrderDataStream {
public:
    using ByteOrderDataStream::ByteOrderDataStream;

    inline bool eofRead() const { return m_buf.eofRead(); }

    template<typename T>
    inline ByteOrderDataStreamReader& operator>>(T& data)
    {
        constexpr size_t size = sizeof(T);
        static_assert(std::is_arithmetic<T>::value, "Only scalar data streaming is allowed.");
        const uint8_t* bufferP = m_buf.posRead(size);
        if (!bufferP)
            return *this;

        read<size>(reinterpret_cast<uint8_t*>(&data), bufferP, this->getTypeMask<T>());
        m_buf.markRead(size);
        return *this;
    }
    template<class T>
    inline T readScalar()
    {
        T ret = T();
        *this >> ret;
        return ret;
    }

    inline ByteOrderDataStreamReader& operator>>(HasRead auto& data)
    {
        data.readBinary(*this);
        return *this;
    }

    template<typename T>
    inline ByteOrderDataStreamReader& operator>>(std::vector<T>& data)
    {
        auto size = readSize();
        data.resize(size);
        for (auto& element : data)
            *this >> element;
        return *this;
    }

    template<typename T>
    inline ByteOrderDataStreamReader& operator>>(std::deque<T>& data)
    {
        auto size = readSize();
        data.resize(size);
        for (auto& element : data)
            *this >> element;
        return *this;
    }

    template<typename K, typename V>
    inline ByteOrderDataStreamReader& operator>>(std::map<K, V>& data)
    {
        auto size = readSize();
        for (uint32_t i = 0; i < size; ++i) {
            K key;
            V value;
            *this >> key >> value;
            data[key] = value;
        }

        return *this;
    }

    template<size_t size>
    inline ByteOrderDataStreamReader& operator>>(std::array<uint8_t, size>& data)
    {
        this->readBlock(data.data(), size);
        return *this;
    }

    std::string readPascalString()
    {
        auto size = readSize();
        if (this->eofRead())
            return {};

        std::string str;
        str.resize(size);
        readBlock((uint8_t*) str.data(), size);

        return eofRead() ? std::string{} : str;
    }
    void readBlock(uint8_t* data, ptrdiff_t size)
    {
        const uint8_t* start = m_buf.posRead(size);

        memcpy(data, start, size);
        m_buf.markRead(size);
        m_buf.checkRemain(0);
    }
    void readBlock(char* data, ptrdiff_t size)
    {
        readBlock(reinterpret_cast<uint8_t*>(data), size);
    }
    template<size_t strSize>
    void readStringWithGarbagePadding(std::string& str, std::vector<uint8_t>& strGarbagePadding)
    {
        std::array<char, strSize + 1> strBuffer{};
        this->readBlock(strBuffer.data(), strSize);
        str = strBuffer.data();
        if (str.size() < strSize - 1) {
            strGarbagePadding.resize(strSize - str.size() - 1);
            std::memcpy(strGarbagePadding.data(), strBuffer.data() + str.size() + 1, strGarbagePadding.size());
            while (!strGarbagePadding.empty() && (*strGarbagePadding.rbegin() == 0))
                strGarbagePadding.pop_back();
        }
    }

    void zeroPadding(ptrdiff_t size)
    {
        m_buf.markRead(size);
        m_buf.checkRemain(0);
    }

    void zeroPaddingChecked(ptrdiff_t size, bool check)
    {
        if (!check)
            return zeroPadding(size);
        for (ptrdiff_t i = 0; i < size; ++i) {
            const auto byte = this->readScalar<uint8_t>();
            if (byte != 0)
                throw std::runtime_error("zeroPadding contains non-zero byte at [" + std::to_string(i) + "] = " + std::to_string(int(byte)));
        }
    }

    void readBits(std::vector<uint8_t>& bitArray, bool invert = false, bool inverseArrayIndex = false)
    {
        const uint8_t invertByte = invert;
        const size_t  bitCount   = bitArray.size();
        const size_t  byteCount  = (bitCount + 7) / 8;
        for (size_t byte = 0; byte < byteCount; ++byte) {
            const size_t  bitOffset      = byte * 8;
            const size_t  bitCountInByte = std::min(size_t(8), bitCount - bitOffset);
            const uint8_t mask           = this->readScalar<uint8_t>();
            for (size_t bit = 0; bit < bitCountInByte; ++bit) {
                const uint8_t flag          = static_cast<bool>(mask & (1 << bit));
                const size_t  bitArrayIndex = bitOffset + bit;

                bitArray[inverseArrayIndex ? (bitCount - bitArrayIndex - 1) : bitArrayIndex] = flag ^ invertByte;
            }
        }
    }

    size_t readSize()
    {
        size_t size = [this]() -> size_t {
            switch (m_containerSizeBytes) {
                case 1:
                    return readScalar<uint8_t>();
                case 2:
                    return readScalar<uint16_t>();
                case 4:
                    return readScalar<uint32_t>();
                case 8:
                    return static_cast<size_t>(readScalar<uint64_t>());
                default:
                    throw std::runtime_error("Invalid sizeof size is set:" + std::to_string(m_containerSizeBytes));
                    break;
            }
        }();
        if (size > (size_t) getBuffer().getRemainRead())
            throw std::runtime_error("Got size that exceedes remaining buffer!");
        return size;
    }

private:
    template<size_t bytes>
    inline void read(uint8_t*, const uint8_t*, uint_fast8_t) const
    {
        static_assert(bytes <= 8, "Unknown size");
    }
};

class ByteOrderDataStreamWriter : public ByteOrderDataStream {
public:
    using ByteOrderDataStream::ByteOrderDataStream;

    template<typename T>
    inline ByteOrderDataStreamWriter& operator<<(const T& data)
    {
        constexpr size_t size = sizeof(T);
        static_assert(std::is_arithmetic<T>::value, "Only scalar data streaming is allowed.");

        uint8_t* bufferP = m_buf.posWrite(size);

        write<size>(reinterpret_cast<const uint8_t*>(&data), bufferP, this->getTypeMask<T>());
        m_buf.markWrite(size);
        return *this;
    }

    template<typename T>
    inline ByteOrderDataStreamWriter& operator<<(const std::vector<T>& data)
    {
        writeSize(data.size());
        for (const auto& element : data)
            *this << element;
        return *this;
    }
    template<typename T>
    inline ByteOrderDataStreamWriter& operator<<(const std::deque<T>& data)
    {
        writeSize(data.size());
        for (const auto& element : data)
            *this << element;
        return *this;
    }

    template<typename K, typename V>
    inline ByteOrderDataStreamWriter& operator<<(const std::map<K, V>& data)
    {
        writeSize(data.size());
        for (const auto& elementPair : data)
            *this << elementPair.first << elementPair.second;
        return *this;
    }

    inline ByteOrderDataStreamWriter& operator<<(const HasWrite auto& data)
    {
        data.writeBinary(*this);
        return *this;
    }

    template<size_t size>
    inline ByteOrderDataStreamWriter& operator<<(const std::array<uint8_t, size>& data)
    {
        this->writeBlock(data.data(), size);
        return *this;
    }

    template<typename T>
    inline void writeToOffset(const T& data, ptrdiff_t writeOffset);

    /// Read/write strings with size.
    void writePascalString(const std::string& str)
    {
        writeSize(str.size());

        writeBlock(str.data(), str.size());
    }
    /// Read/write raw data blocks.
    void writeBlock(const uint8_t* data, ptrdiff_t size)
    {
        uint8_t* start = m_buf.posWrite(size);

        memcpy(start, data, size);
        m_buf.markWrite(size);
        m_buf.checkRemain(0);
    }
    void writeBlock(const char* data, ptrdiff_t size)
    {
        writeBlock(reinterpret_cast<const uint8_t*>(data), size);
    }
    template<size_t sizeStr>
    void writeStringWithGarbagePadding(const std::string& str, const std::vector<uint8_t>& strGarbagePadding)
    {
        if (str.size() + strGarbagePadding.size() > sizeStr)
            throw std::runtime_error("String must be at most " + std::to_string(sizeStr) + " symbols: " + str);

        std::array<char, sizeStr + 1> strBuffer{};
        std::memcpy(strBuffer.data(), str.data(), str.size());
        if (strGarbagePadding.size())
            std::memcpy(strBuffer.data() + str.size() + 1, strGarbagePadding.data(), strGarbagePadding.size());
        this->writeBlock(strBuffer.data(), sizeStr);
    }

    void zeroPadding(size_t count)
    {
        uint8_t* start = m_buf.posWrite(count);
        if (!start)
            return;
        std::memset(start, 0, count);
        m_buf.markWrite(count);
        m_buf.checkRemain(0);
    }

    void writeBits(const std::vector<uint8_t>& bitArray, bool invert = false, bool inverseArrayIndex = false)
    {
        const uint8_t invertByte = invert;
        const size_t  bitCount   = bitArray.size();
        const size_t  byteCount  = (bitCount + 7) / 8;
        for (size_t byte = 0; byte < byteCount; ++byte) {
            const size_t bitOffset      = byte * 8;
            const size_t bitCountInByte = std::min(size_t(8), bitCount - bitOffset);
            uint8_t      mask           = 0;
            for (size_t bit = 0; bit < bitCountInByte; ++bit) {
                const size_t  bitArrayIndex = bitOffset + bit;
                const uint8_t flag          = bitArray[inverseArrayIndex ? (bitCount - bitArrayIndex - 1) : bitArrayIndex] ^ invertByte;
                if (flag)
                    mask |= (1 << bit);
            }
            *this << mask;
        }
    }

    void writeSize(size_t size)
    {
        switch (m_containerSizeBytes) {
            case 1:
                if (size > std::numeric_limits<uint8_t>::max())
                    throw std::runtime_error("Container is too large:" + std::to_string(size));
                *this << static_cast<uint8_t>(size);
                return;
            case 2:
                if (size > std::numeric_limits<uint16_t>::max())
                    throw std::runtime_error("Container is too large:" + std::to_string(size));
                *this << static_cast<uint16_t>(size);
                return;
            case 4:
                if (size > std::numeric_limits<uint32_t>::max())
                    throw std::runtime_error("Container is too large:" + std::to_string(size));
                *this << static_cast<uint32_t>(size);
                return;
            case 8:
                *this << static_cast<uint64_t>(size);
                return;
            default:
                break;
        }
        throw std::runtime_error("Invalid sizeof size is set:" + std::to_string(m_containerSizeBytes));
    }

private:
    template<size_t bytes>
    inline void write(const uint8_t*, uint8_t*, uint_fast8_t) const
    {
        static_assert(bytes <= 8, "Unknown size");
    }
};

template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<uint64_t>() const
{
    return m_maskInt64;
}
template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<int64_t>() const
{
    return m_maskInt64;
}
template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<uint32_t>() const
{
    return m_maskInt32;
}
template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<int32_t>() const
{
    return m_maskInt32;
}
template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<uint16_t>() const
{
    return m_maskInt16;
}
template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<int16_t>() const
{
    return m_maskInt16;
}
template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<double>() const
{
    return m_maskDouble;
}
template<>
inline uint_fast8_t ByteOrderDataStream::getTypeMask<float>() const
{
    return m_maskFloat;
}

template<>
inline void ByteOrderDataStreamReader::read<8>(uint8_t* data, const uint8_t* buffer, uint_fast8_t mask) const
{
    data[mask ^ 0] = *buffer++;
    data[mask ^ 1] = *buffer++;
    data[mask ^ 2] = *buffer++;
    data[mask ^ 3] = *buffer++;
    data[mask ^ 4] = *buffer++;
    data[mask ^ 5] = *buffer++;
    data[mask ^ 6] = *buffer++;
    data[mask ^ 7] = *buffer++;
}

template<>
inline void ByteOrderDataStreamReader::read<4>(uint8_t* data, const uint8_t* buffer, uint_fast8_t mask) const
{
    data[mask ^ 0] = *buffer++;
    data[mask ^ 1] = *buffer++;
    data[mask ^ 2] = *buffer++;
    data[mask ^ 3] = *buffer++;
}
template<>
inline void ByteOrderDataStreamReader::read<2>(uint8_t* data, const uint8_t* buffer, uint_fast8_t mask) const
{
    data[mask ^ 0] = *buffer++;
    data[mask ^ 1] = *buffer++;
}
template<>
inline void ByteOrderDataStreamReader::read<1>(uint8_t* data, const uint8_t* buffer, uint_fast8_t mask) const
{
    data[mask ^ 0] = *buffer++;
}

template<>
inline void ByteOrderDataStreamWriter::write<8>(const uint8_t* data, uint8_t* buffer, uint_fast8_t mask) const
{
    *buffer++ = data[mask ^ 0];
    *buffer++ = data[mask ^ 1];
    *buffer++ = data[mask ^ 2];
    *buffer++ = data[mask ^ 3];
    *buffer++ = data[mask ^ 4];
    *buffer++ = data[mask ^ 5];
    *buffer++ = data[mask ^ 6];
    *buffer++ = data[mask ^ 7];
}
template<>
inline void ByteOrderDataStreamWriter::write<4>(const uint8_t* data, uint8_t* buffer, uint_fast8_t mask) const
{
    *buffer++ = data[mask ^ 0];
    *buffer++ = data[mask ^ 1];
    *buffer++ = data[mask ^ 2];
    *buffer++ = data[mask ^ 3];
}
template<>
inline void ByteOrderDataStreamWriter::write<2>(const uint8_t* data, uint8_t* buffer, uint_fast8_t mask) const
{
    *buffer++ = data[mask ^ 0];
    *buffer++ = data[mask ^ 1];
}
template<>
inline void ByteOrderDataStreamWriter::write<1>(const uint8_t* data, uint8_t* buffer, uint_fast8_t mask) const
{
    *buffer++ = data[mask ^ 0];
}

template<>
inline ByteOrderDataStreamReader& ByteOrderDataStreamReader::operator>>(std::string& data)
{
    data = readPascalString();
    return *this;
}
template<>
inline ByteOrderDataStreamWriter& ByteOrderDataStreamWriter::operator<<(const std::string& data)
{
    writePascalString(data);
    return *this;
}

template<typename T>
void ByteOrderDataStreamWriter::writeToOffset(const T& data, ptrdiff_t writeOffset)
{
    write<sizeof(T)>(reinterpret_cast<const uint8_t*>(&data), m_buf.begin() + writeOffset, this->getTypeMask<T>());
}

}
