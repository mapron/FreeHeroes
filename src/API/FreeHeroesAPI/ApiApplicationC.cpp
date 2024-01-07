/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ApiApplicationC.h"
#include "ApiApplication.hpp"

using namespace FreeHeroes;
namespace {
FHAppHandle g_globalHandle = nullptr;

ApiApplicationNoexcept* toApp(FHAppHandle app)
{
    return reinterpret_cast<ApiApplicationNoexcept*>(app);
}

FHAppHandle toHandle(ApiApplicationNoexcept* app)
{
    return reinterpret_cast<FHAppHandle>(app);
}

}

FHApiPointers fh_get_api_pointers_v1()
{
    return FHApiPointers{
        .create                = &fh_create_v1,
        .destroy               = &fh_destroy_v1,
        .get_last_error        = &fh_get_last_error_v1,
        .get_last_output       = &fh_get_last_output_v1,
        .init                  = &fh_init_v1,
        .convert_lod           = &fh_convert_lod_v1,
        .map_load              = &fh_map_load_v1,
        .get_map_version       = &fh_get_map_version_v1,
        .get_map_width         = &fh_get_map_width_v1,
        .get_map_height        = &fh_get_map_height_v1,
        .get_map_depth         = &fh_get_map_depth_v1,
        .get_map_tile_size     = &fh_get_map_tile_size_v1,
        .map_derandomize       = &fh_map_derandomize_v1,
        .set_map_render_window = &fh_set_map_render_window_v1,
        .map_prepare_render    = &fh_map_prepare_render_v1,
        .map_paint             = &fh_map_paint_v1,
        .get_map_paint_result  = &fh_get_map_paint_result_v1,
    };
}

FHAppHandle fh_create_v1(void)
{
    return toHandle(new ApiApplicationNoexcept);
}

void fh_destroy_v1(FHAppHandle app)
{
    return delete toApp(app);
}

FHString fh_get_last_error_v1(FHAppHandle app)
{
    return toApp(app)->getLastError();
}
FHString fh_get_last_output_v1(FHAppHandle app)
{
    return toApp(app)->getLastOutput();
}
FHResult fh_init_v1(FHAppHandle app, FHString mainResourcePath, FHString userResourcePath)
{
    if (!app)
        return 0;
    return toApp(app)->init(mainResourcePath, userResourcePath);
}
FHResult fh_convert_lod_v1(FHAppHandle app, FHString lodPath, FHString appResourcePath, FHString userResourcePath)
{
    if (!app)
        return 0;
    return toApp(app)->convertLoD(lodPath, appResourcePath, userResourcePath);
}

FHResult fh_map_load_v1(FHAppHandle app, FHString mapFile)
{
    if (!app)
        return 0;
    return toApp(app)->loadMap(mapFile);
}

int fh_get_map_version_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->getMapInfo().m_version;
}

int fh_get_map_width_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->getMapInfo().m_width;
}

int fh_get_map_height_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->getMapInfo().m_height;
}

int fh_get_map_depth_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->getMapInfo().m_depth;
}
int fh_get_map_tile_size_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->getMapInfo().m_tileSize;
}

FHResult fh_map_derandomize_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->derandomize();
}

FHResult fh_set_map_render_window_v1(FHAppHandle app, int x, int y, int z, int width, int height)
{
    if (!app)
        return 0;
    return toApp(app)->setRenderWindow({ .m_x = x, .m_y = y, .m_z = z, .m_width = width, .m_height = height });
}

FHResult fh_map_prepare_render_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->prepareRender();
}

FHResult fh_map_paint_v1(FHAppHandle app)
{
    if (!app)
        return 0;
    return toApp(app)->paint();
}

FHBitmap fh_get_map_paint_result_v1(FHAppHandle app)
{
    if (!app)
        return nullptr;
    return toApp(app)->getRGBA();
}

void fh_global_create_v1()
{
    if (g_globalHandle)
        fh_destroy_v1(g_globalHandle);
    g_globalHandle = fh_create_v1();
}

void fh_global_destroy_v1(void)
{
    return fh_destroy_v1(g_globalHandle);
}
FHString fh_global_get_last_error_v1(void)
{
    return fh_get_last_error_v1(g_globalHandle);
}
FHString fh_global_get_last_output_v1(void)
{
    return fh_get_last_output_v1(g_globalHandle);
}
FHResult fh_global_init_v1(FHString mainResourcePath, FHString userResourcePath)
{
    return fh_init_v1(g_globalHandle, mainResourcePath, userResourcePath);
}
FHResult fh_global_convert_lod_v1(FHString lodPath, FHString appResourcePath, FHString userResourcePath)
{
    return fh_convert_lod_v1(g_globalHandle, lodPath, appResourcePath, userResourcePath);
}
FHResult fh_global_map_load_v1(FHString mapFile)
{
    return fh_map_load_v1(g_globalHandle, mapFile);
}
int fh_global_get_map_version_v1(void)
{
    return fh_get_map_version_v1(g_globalHandle);
}
int fh_global_get_map_width_v1(void)
{
    return fh_get_map_width_v1(g_globalHandle);
}
int fh_global_get_map_height_v1(void)
{
    return fh_get_map_height_v1(g_globalHandle);
}
int fh_global_get_map_depth_v1(void)
{
    return fh_get_map_depth_v1(g_globalHandle);
}
int fh_global_get_map_tile_size_v1()
{
    return fh_get_map_tile_size_v1(g_globalHandle);
}

FHResult fh_global_map_derandomize_v1(void)
{
    return fh_map_derandomize_v1(g_globalHandle);
}
FHResult fh_global_set_map_render_window_v1(int x, int y, int z, int width, int height)
{
    return fh_set_map_render_window_v1(g_globalHandle, x, y, z, width, height);
}
FHResult fh_global_map_prepare_render_v1(void)
{
    return fh_map_prepare_render_v1(g_globalHandle);
}
FHResult fh_global_map_paint_v1(void)
{
    return fh_map_paint_v1(g_globalHandle);
}
FHBitmap fh_global_get_map_paint_result_v1(void)
{
    return fh_get_map_paint_result_v1(g_globalHandle);
}

FHApiPointersGlobal fh_global_get_api_pointers_v1()
{
    return FHApiPointersGlobal{
        .create                = &fh_global_create_v1,
        .destroy               = &fh_global_destroy_v1,
        .get_last_error        = &fh_global_get_last_error_v1,
        .get_last_output       = &fh_global_get_last_output_v1,
        .init                  = &fh_global_init_v1,
        .convert_lod           = &fh_global_convert_lod_v1,
        .map_load              = &fh_global_map_load_v1,
        .get_map_version       = &fh_global_get_map_version_v1,
        .get_map_width         = &fh_global_get_map_width_v1,
        .get_map_height        = &fh_global_get_map_height_v1,
        .get_map_depth         = &fh_global_get_map_depth_v1,
        .get_map_tile_size     = &fh_global_get_map_tile_size_v1,
        .map_derandomize       = &fh_global_map_derandomize_v1,
        .set_map_render_window = &fh_global_set_map_render_window_v1,
        .map_prepare_render    = &fh_global_map_prepare_render_v1,
        .map_paint             = &fh_global_map_paint_v1,
        .get_map_paint_result  = &fh_global_get_map_paint_result_v1,
    };
}
