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
    struct RenderWindow {
        int m_x      = 0;
        int m_y      = 0;
        int m_z      = 0;
        int m_width  = 0;
        int m_height = 0;
    };
    using Bitmap = const uint8_t*;

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
    void setRenderWindow(const RenderWindow& renderWindow) noexcept;
    void prepareRender();

    void   paint();
    Bitmap getRGBA() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class FREEHEROESAPI_EXPORT ApiApplicationNoexcept {
public:
    using MapInfo      = ApiApplication::MapInfo;
    using Bitmap       = ApiApplication::Bitmap;
    using RenderWindow = ApiApplication::RenderWindow;

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
    bool setRenderWindow(const RenderWindow& renderWindow) noexcept;
    bool prepareRender() noexcept;

    bool   paint() noexcept;
    Bitmap getRGBA() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
