/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MernelPlatform/SharedLibLoader.hpp"
#include "MernelPlatform/ScopeExit.hpp"
#include "MernelPlatform/ByteOrderStream.hpp"
#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/Profiler.hpp"

#include "ApiApplicationC.h"

#include <iostream>
#include <thread>

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

bool saveBMP(const std::string& path, const uint8_t* rgbapixels, int w, int h)
{
    // Function to round an int to a multiple of 4

    ByteArrayHolder binaryBuffer;
    {
        ByteOrderBuffer           bobuffer(binaryBuffer);
        ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::s_littleEndian);
        writer << uint8_t('B') << uint8_t('M');
        std::vector<uint32_t> header{
            0,    // filesize
            0,    // reserved;
            0x36, //fileoffset_to_pixelarray
            0x28, // dibheadersize
            (uint32_t) w,
            (uint32_t) h
        };
        for (auto n : header)
            writer << n;
        uint16_t planes = 1;
        uint16_t bpp    = 24;
        uint32_t dpi    = 0x130B; //2835 , 72 DPI
        uint32_t pix    = w * h * 3;

        writer << planes << bpp;

        header = std::vector<uint32_t>{
            0, // compression
            pix,
            dpi,
            dpi,
            0, // numcolorspallette
            0, // mostimpcolor
        };

        for (auto n : header)
            writer << n;

        // Pad the width of the destination to a multiple of 4
        const int strideSize = w * 3;
        const int padding    = strideSize % 4 == 0 ? 0 : 4 - strideSize % 4;

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const uint8_t* src = (rgbapixels + ((h - y - 1) * w + x) * 4);
                writer << src[2] << src[1] << src[0]; // BMP have BGR orger.
            }
            writer.zeroPadding(padding);
        }
        writer.getBuffer().setOffsetWrite(2);
        writer << uint32_t(writer.getBuffer().getSize());
    }
    return writeFileFromHolderNoexcept(path, binaryBuffer);
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: FreeHeroesTest D:/Games/Heroes3_HotA D:/tmp [plugin/root/]\n";
        return 1;
    }

    Mernel::ScopeTimer launchTimer;

    const std::string heroesPath = argv[1];
    const std::string tmpPath    = argv[2];
    const std::string pluginRoot = argc >= 4 ? argv[3] : ".";
    std::cout << "Using heroesPath=" << heroesPath << ", tmpPath=" << tmpPath << ", pluginRoot=" << pluginRoot << "\n";

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

    const std::string lodPath1 = heroesPath + "/Data/H3sprite.lod";
    const std::string lodPath2 = heroesPath + "/Data/HotA.lod";

    if (!checkApiCall("convert_lod 1", [&api, lodPath1] { return api.convert_lod(lodPath1.c_str()); }))
        return 1;
    if (!checkApiCall("convert_lod 2", [&api, lodPath2] { return api.convert_lod(lodPath2.c_str()); }))
        return 1;

    // we don't need to call reinit every time.
    // in this example we call it to make sure resources are parsed after fresh first call to convert_lod();
    if (!checkApiCall("reinit", [&api] { return api.reinit(); }))
        return 1;

    const std::string mapPath = heroesPath + "/Maps/[HotA] Air Supremacy.h3m"; // 1.7.0 map!

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

    const int paintXoffset = 13;
    const int paintYoffset = 6;
    const int paintZoffset = 0; // surface

    const int paintWidth  = 10;
    const int paintHeight = 8;

    if (!checkApiCall("set_map_render_window", [=, &api] { return api.set_map_render_window(paintXoffset, paintYoffset, paintZoffset, paintWidth, paintHeight); }))
        return 1;

    if (!checkApiCall("map_prepare_render", [&api, mapPath] { return api.map_prepare_render(); }))
        return 1;

    if (!checkApiCall("map_paint 1", [=, &api] { return api.map_paint(); }))
        return 1;

    auto* result = api.get_map_paint_result();
    if (!result) {
        std::cerr << "No paint result!\n"; // we probably shouldn't get there but just in case.
        return 1;
    }
    std::cout << "Total run time from app launch to completed first paint: " << (launchTimer.elapsedUS() / 1000) << " ms.\n";
    if (!saveBMP(tmpPath + "/out1.bmp", result, paintWidth * tileSize, paintHeight * tileSize)) {
        std::cerr << "Failed to save output bitmap\n";
        return 1;
    }
    // repeat for getting ext animation frame
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    if (!checkApiCall("map_paint 2", [=, &api] { return api.map_paint(); }))
        return 1;
    result = api.get_map_paint_result();
    if (!result) {
        std::cerr << "No paint result!\n"; // we probably shouldn't get there but just in case.
        return 1;
    }
    if (!saveBMP(tmpPath + "/out2.bmp", result, paintWidth * tileSize, paintHeight * tileSize)) {
        std::cerr << "Failed to save output bmitmap\n";
        return 1;
    }

    return 0;
}
