/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FileFormatJson.hpp"

#include "Logger.hpp"
#include "Profiler.hpp"

#include <sstream>
#include <iterator>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/filestream.h>

namespace FreeHeroes::Core {

class JsonStreamOut {
public:
    JsonStreamOut(std::string& output)
        : m_output(output)
    {}

    char   Peek() const { return 0; }
    char   Take() { return 0; }
    size_t Tell() const { return 0; }

    void Put(char c) { m_output += c; }

    char*  PutBegin() { return 0; }
    size_t PutEnd(char*) { return 0; }

private:
    std::string& m_output;
};

void jsonToPropery(PropertyTree& data, rapidjson::Value& input)
{
    switch (input.GetType()) {
        case rapidjson::kNullType:
        {
            data = {};
        } break;
        case rapidjson::kFalseType:
        {
            data = PropertyTreeScalar(false);
        } break;
        case rapidjson::kTrueType:
        {
            data = PropertyTreeScalar(true);
        } break;
        case rapidjson::kObjectType:
        {
            data.convertToMap();
            auto& m = data.getMap();
            for (auto keyIt = input.MemberBegin(); keyIt != input.MemberEnd(); ++keyIt) {
                std::string key(keyIt->name.GetString(), keyIt->name.GetStringLength());
                jsonToPropery(m[key], keyIt->value);
            }
        } break;
        case rapidjson::kArrayType:
        {
            data.convertToList();
            auto& l = data.getList();
            l.resize(std::distance(input.Begin(), input.End()));
            size_t index = 0;
            for (auto nodeIt = input.Begin(); nodeIt != input.End(); ++nodeIt) {
                jsonToPropery(l[index++], *nodeIt);
            }
        } break;
        case rapidjson::kStringType:
        {
            std::string inputString(input.GetString(), input.GetStringLength());
            data = PropertyTreeScalar(std::move(inputString));
        } break;
        case rapidjson::kNumberType:
        {
            if (input.IsDouble())
                data = PropertyTreeScalar(input.GetDouble());
            else if (input.IsInt())
                data = PropertyTreeScalar(std::int64_t(input.GetInt()));
            else if (input.IsInt64())
                data = PropertyTreeScalar(std::int64_t(input.GetInt64()));
            else if (input.IsUint())
                data = PropertyTreeScalar(std::int64_t(input.GetUint()));
            else if (input.IsUint64())
                data = PropertyTreeScalar(std::int64_t(input.GetUint64()));
        } break;
    }
}

void propertyToJson(const PropertyTree& data, rapidjson::Value& json, rapidjson::Document::AllocatorType& allocator)
{
    if (data.isNull()) {
    } else if (data.isList()) {
        json.SetArray();
        for (const PropertyTree& child : data.getList()) {
            rapidjson::Value tempValue;
            propertyToJson(child, tempValue, allocator);
            json.PushBack(tempValue, allocator);
        }
    } else if (data.isMap()) {
        json.SetObject();
        for (const auto& p : data.getMap()) {
            const PropertyTree& child = p.second;
            rapidjson::Value    tempValue;
            propertyToJson(child, tempValue, allocator);
            json.AddMember(p.first.c_str(), allocator, tempValue, allocator);
        }
    } else if (data.isScalar()) {
        const auto& scalar = data.getScalar();
        if (scalar.isBool())
            json.SetBool(scalar.toBool());
        if (scalar.isInt())
            json.SetInt64(scalar.toInt());
        if (scalar.isDouble())
            json.SetDouble(scalar.toDouble());
        if (scalar.isString()) {
            const auto s = scalar.toString();
            json.SetString(s.data(), s.size(), allocator);
        }
    }
}

bool readJsonFromBuffer(const std::string& buffer, PropertyTree& data) noexcept(true)
{
    rapidjson::Document input;
    const char*         dataPtr = buffer.data();
    if (buffer.starts_with(std::string_view("\xef\xbb\xbf", 3)))
        dataPtr += 3;

    auto& res = input.Parse<0>(dataPtr);
    if (res.HasParseError()) {
        Logger(Logger::Err) << res.GetParseError() << " (" << res.GetErrorOffset() << ")";
        return false;
    }

    if (!input.IsObject() && !input.IsArray())
        return false;

    data = PropertyTree{};
    jsonToPropery(data, input);

    return true;
}

bool writeJsonToBuffer(std::string& buffer, const PropertyTree& data) noexcept(true)
{
    if (data.isNull()) {
        return false;
    }
    rapidjson::Document json;
    propertyToJson(data, json, json.GetAllocator());

    JsonStreamOut                    outStream(buffer);
    rapidjson::Writer<JsonStreamOut> writer(outStream);
    json.Accept(writer);

    return true;
}

PropertyTree readJsonFromBufferThrow(const std::string& buffer) noexcept(false)
{
    PropertyTree result;
    if (!readJsonFromBuffer(buffer, result))
        throw std::runtime_error("Failed to read JSON");
    return result;
}

std::string writeJsonToBufferThrow(const PropertyTree& data) noexcept(false)
{
    std::string buffer;
    if (!writeJsonToBuffer(buffer, data))
        throw std::runtime_error("Failed to write JSON");
    return buffer;
}

}
