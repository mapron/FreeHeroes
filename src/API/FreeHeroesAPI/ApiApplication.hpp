/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#pragma once

#include <memory>
#include <string>

#include "FreeHeroesAPIExport.hpp"

namespace FreeHeroes {

class FREEHEROESAPI_EXPORT ApiApplication {
public:
    struct MapInfo {
        int m_version  = 0;
        int m_width    = 0;
        int m_height   = 0;
        int m_depth    = 0;
        int m_tileSize = 0;
    };
    using RGBAArray = const uint32_t*;

public:
    ApiApplication() noexcept;
    ~ApiApplication() noexcept;

    const std::string& getLastOutput() const noexcept;
    void               clearOutput() noexcept;

    void init(const std::string& appResourcePath, const std::string& userResourcePath) noexcept(false);
    void convertLoD(const std::string& lodPath, const std::string& userResourcePath) noexcept(false);
    void loadMap(const std::string& mapPath) noexcept(false);

    const MapInfo& getMapInfo() const noexcept;

    void derandomize();
    void prepareRender();

    void      paint(int x, int y, int z, int width, int height);
    RGBAArray getRGBA() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class FREEHEROESAPI_EXPORT ApiApplicationNoexcept {
public:
    using MapInfo   = ApiApplication::MapInfo;
    using RGBAArray = ApiApplication::RGBAArray;

public:
    ApiApplicationNoexcept() noexcept;
    ~ApiApplicationNoexcept() noexcept;

    const char* getLastError() const noexcept;
    const char* getLastOutput() const noexcept;

    bool init(const char* appResourcePath, const char* userResourcePath) noexcept;
    bool convertLoD(const char* lodPath, const char* userResourcePath) noexcept;
    bool loadMap(const char* mapPath) noexcept;

    const MapInfo& getMapInfo() const noexcept;

    bool derandomize() noexcept;
    bool prepareRender() noexcept;

    bool      paint(int x, int y, int z, int width, int height) noexcept;
    RGBAArray getRGBA() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
