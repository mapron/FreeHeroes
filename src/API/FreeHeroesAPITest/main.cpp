/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MernelPlatform/SharedLibLoader.hpp"
#include "MernelPlatform/ScopeExit.hpp"

#include "ApiApplicationC.h"

#include <iostream>

using namespace Mernel;

namespace {
const std::string g_pluginName = "FreeHeroesAPI";
#ifndef NDEBUG
const std::string g_pluginSuffix = "D";
#else
const std::string g_pluginSuffix    = "";
#endif

#ifdef _WIN32
const std::string g_pluginExtension = ".dll";
#elif defined(__APPLE__)
const std::string g_pluginExtension = ".dylib";
#else
const std::string g_pluginExtension = ".so";
#endif
const std::string g_pluginFullName = g_pluginName + g_pluginSuffix + g_pluginExtension;
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::cerr << "Usage: FreeHeroesTest D:/Games/Heroes3_HotA D:/tmp /plugin/root/\n";
        return 1;
    }

    const std::string heroesPath = argv[1];
    const std::string tmpPath    = argv[2];
    const std::string pluginRoot = argv[3];
    std::cout << "Using heroesPath=" << heroesPath << ", tmpPath=" << tmpPath << ", pluginRoot=" << pluginRoot << "\n";

#if 1
    SharedLibLoader loader;
    if (!loader.open(pluginRoot + "/" + g_pluginFullName)) {
        std::cerr << "Failed to load: " << g_pluginFullName << ", " << loader.getLastError() << "\n";
        return 1;
    }

    MERNEL_SCOPE_EXIT([&] { loader.close(); });
    void* address = loader.getSymbol("fh_global_get_api_pointers_v1");
    if (!address) {
        std::cerr << "Failed to find fh_global_get_api_pointers_v1\n";
        return 1;
    }
    FHApiPointersGlobal api = reinterpret_cast<fh_global_get_api_pointers_v1_f>(address)();

#else
    FHApiPointersGlobal api = fh_global_get_api_pointers_v1();
#endif
    if (!api.create) {
        std::cerr << "Something really bad with getting address struct!\n";
        return 1;
    }

    auto checkApiCall = [&api](const char* name, auto&& callback) -> bool {
        if (!callback()) {
            std::cerr << "Method api.'" << name << "' failed, error: " << api.get_last_error() << ", output: \n"
                      << api.get_last_output();
            return false;
        }
        std::cout << "Method api.'" << name << "' succeeded with output: \n"
                  << api.get_last_output();

        return true;
    };

    if (!checkApiCall("create", [&api] { api.create(); return true; }))
        return 1;
    MERNEL_SCOPE_EXIT([&] { api.destroy(); });

    const std::string appPath  = "./gameResources";
    const std::string userPath = tmpPath;
    if (!checkApiCall("init", [&api, appPath, userPath] { return api.init(appPath.c_str(), userPath.c_str()); }))
        return 1;

    const std::string lodPath1 = heroesPath + "/Data/H3bitmap.lod";
    const std::string lodPath2 = heroesPath + "/Data/HotA.lod";

    if (!checkApiCall("convert_lod 1", [&api, lodPath1, userPath] { return api.convert_lod(lodPath1.c_str(), userPath.c_str()); }))
        return 1;
    if (!checkApiCall("convert_lod 2", [&api, lodPath2, userPath] { return api.convert_lod(lodPath2.c_str(), userPath.c_str()); }))
        return 1;

    const std::string mapPath = heroesPath + "/Data/[HotA] Air Supremacy.h3m"; // 1.7.0 map!

    if (!checkApiCall("map_load", [&api, mapPath] { return api.map_load(mapPath.c_str()); }))
        return 1;

    const int version  = api.get_map_version();
    const int width    = api.get_map_width();
    const int height   = api.get_map_height();
    const int depth    = api.get_map_depth();
    const int tileSize = api.get_map_tile_size();
    if (version == 0) {
        std::cerr << "Invalid map version\n"; // we probably shouldn't get there but just in case.
        return 1;
    }
    std::cout << "Loaded map version = " << (version == 1 ? "SoD" : "HotA") << ", size=" << width << "x" << height << "x" << depth << ", tileSize=" << tileSize << "\n";

    if (!checkApiCall("map_derandomize", [&api, mapPath] { return api.map_derandomize(); }))
        return 1;
    if (!checkApiCall("map_prepare_render", [&api, mapPath] { return api.map_prepare_render(); }))
        return 1;

    const int paintXoffset = 10;
    const int paintYoffset = 10;
    const int paintZoffset = 0; // surface

    const int paintWidth  = 5;
    const int paintHeight = 10;

    if (!checkApiCall("map_paint", [=, &api] { return api.map_paint(paintXoffset, paintYoffset, paintZoffset, paintWidth, paintHeight); }))
        return 1;

    FHRgbaArray result = api.get_map_paint_result();
    if (!result) {
        std::cerr << "No paint result!\n"; // we probably shouldn't get there but just in case.
        return 1;
    }
    for (int y = 0; y < paintHeight; ++y) {
        for (int x = 0; x < paintWidth; ++x) {
            std::cout << "Tile pixel data at (" << x << ", " << y << "):\n";
            for (int ypixel = 0; ypixel < tileSize; ++ypixel) {
                for (int xpixel = 0; xpixel < tileSize; ++xpixel) {
                    const uint32_t* rgba = result + (y * tileSize + ypixel) * paintWidth + (x * tileSize + xpixel);
                    std::cout << std::setw(8) << std::hex << std::setfill('0') << *rgba << " ";
                }

                std::cout << "\n";
            }
            std::cout << "\n";
        }
    }

    return 0;
}
