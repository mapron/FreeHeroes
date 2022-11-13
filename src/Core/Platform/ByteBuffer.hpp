/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdint.h>

namespace FreeHeroes {

using StringVector = std::vector<std::string>;
using ByteArray    = std::vector<uint8_t>;

/// Class for explicit sharing blob data.
class ByteArrayHolder {
    std::shared_ptr<ByteArray> p;

public:
    ByteArrayHolder()
        : p(new ByteArray())
    {}

    size_t           size() const { return p.get()->size(); }
    void             resize(size_t size) { return p.get()->resize(size); }
    uint8_t*         data() { return p.get()->data(); }
    const uint8_t*   data() const { return p.get()->data(); }
    ByteArray&       ref() { return *p.get(); }
    const ByteArray& ref() const { return *p.get(); }
};

}
