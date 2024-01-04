/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * This file is licensed under Creative Commons Zero v1.0 Universal; (essentially 'public domain'), see https://spdx.org/licenses/CC0-1.0
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#if defined(_MSC_VER)

#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#ifdef FreeHeroesAPI_EXPORTS
#define FREEHEROESAPI_EXPORT __declspec(dllexport)
#else
#define FREEHEROESAPI_EXPORT __declspec(dllimport)
#endif

#elif defined(__GNUC__)
#define FREEHEROESAPI_EXPORT __attribute__((visibility("default")))
#else
#define FREEHEROESAPI_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef const char*    FHString;
typedef void*          FHAppHandle;
typedef const uint8_t* FHBitmap;
typedef int            FHResult; // 0 = error, 1 = success

/// Create FreeHeroes application instance. never fails.
FREEHEROESAPI_EXPORT FHAppHandle fh_create_v1(void);

/// Deallocare FreeHeroes application instance. never fails.
FREEHEROESAPI_EXPORT void fh_destroy_v1(FHAppHandle app);

/// returns NULL if app is NULL; returns 0-terminated string otherwise. empty if last operation succeeded.
FREEHEROESAPI_EXPORT FHString fh_get_last_error_v1(FHAppHandle app);

/// returns NULL if app is NULL; returns 0-terminated string otherwise.
/// result can be empty or contain 1 or more lines, each terminated with '\n';
FREEHEROESAPI_EXPORT FHString fh_get_last_output_v1(FHAppHandle app);

/**
 * General rules for all API methods:
 * FHResult result is: 0 - error, 1 - success;
 * providing app==NULL results in error;
 * if (app is non-NULL), you can check get_last_error() to get error text if result is 0;
 * if (app is non-NULL), you can check get_last_output() to get output text if result is 0 or 1.
 */

/// Register application resources in both path and create in-game database.
FREEHEROESAPI_EXPORT FHResult fh_init_v1(FHAppHandle app, FHString mainResourcePath, FHString userResourcePath);

/// Convert lodPath=/path/to/bitmap.lod to fhmod format. Result is extracted in userResourcePath
/// @todo: single-file resources?
FREEHEROESAPI_EXPORT FHResult fh_convert_lod_v1(FHAppHandle app, FHString lodPath, FHString userResourcePath);

/// mapFile is path to .h3m or fh .json map file.
FREEHEROESAPI_EXPORT FHResult fh_map_load_v1(FHAppHandle app, FHString mapFile);

/// 0 - error, 1 - SoD, 2 - HotA.
FREEHEROESAPI_EXPORT int fh_get_map_version_v1(FHAppHandle app);

/// returns 0 if map did not initialized properly.
FREEHEROESAPI_EXPORT int fh_get_map_width_v1(FHAppHandle app);
FREEHEROESAPI_EXPORT int fh_get_map_height_v1(FHAppHandle app);
FREEHEROESAPI_EXPORT int fh_get_map_depth_v1(FHAppHandle app);
FREEHEROESAPI_EXPORT int fh_get_map_tile_size_v1(FHAppHandle app);

/// make all random dwelling/monsters not-random.
FREEHEROESAPI_EXPORT FHResult fh_map_derandomize_v1(FHAppHandle app);

/// prepare map sprites
FREEHEROESAPI_EXPORT FHResult fh_map_prepare_render_v1(FHAppHandle app);

/// Need to call get_paint_result for actual data.
FREEHEROESAPI_EXPORT FHResult fh_map_paint_v1(FHAppHandle app, int x, int y, int z, int width, int height);

/// returns NULL if map_paint was not called properly. Otherwise return pointer to array containing width*height*tile_size*tile_size*4 bytes.
FREEHEROESAPI_EXPORT FHBitmap fh_get_map_paint_result_v1(FHAppHandle app);

typedef struct {
    FHAppHandle (*create)(void);
    void (*destroy)(FHAppHandle);
    FHString (*get_last_error)(FHAppHandle);
    FHString (*get_last_output)(FHAppHandle);

    FHResult (*init)(FHAppHandle, FHString, FHString);
    FHResult (*convert_lod)(FHAppHandle, FHString, FHString);
    FHResult (*map_load)(FHAppHandle, FHString);
    int (*get_map_version)(FHAppHandle);
    int (*get_map_width)(FHAppHandle);
    int (*get_map_height)(FHAppHandle);
    int (*get_map_depth)(FHAppHandle);
    int (*get_map_tile_size)(FHAppHandle);
    FHResult (*map_derandomize)(FHAppHandle);
    FHResult (*map_prepare_render)(FHAppHandle);

    FHResult (*map_paint)(FHAppHandle, int x, int y, int z, int width, int height);
    FHBitmap (*get_map_paint_result)(FHAppHandle);
} FHApiPointers;

FREEHEROESAPI_EXPORT FHApiPointers fh_get_api_pointers_v1(void);
typedef FHApiPointers (*fh_get_api_pointers_v1_f)(void);

/**
 * Global counterparts of above methods but without need to pass FHAppHandle everytime.
 */

/// Note: if global_create was already called, then global_destroy will be called on global object.
FREEHEROESAPI_EXPORT void fh_global_create_v1(void);

FREEHEROESAPI_EXPORT void     fh_global_destroy_v1(void);
FREEHEROESAPI_EXPORT FHString fh_global_get_last_error_v1(void);
FREEHEROESAPI_EXPORT FHString fh_global_get_last_output_v1(void);
FREEHEROESAPI_EXPORT FHResult fh_global_init_v1(FHString mainResourcePath, FHString userResourcePath);
FREEHEROESAPI_EXPORT FHResult fh_global_convert_lod_v1(FHString lodPath, FHString userResourcePath);
FREEHEROESAPI_EXPORT FHResult fh_global_map_load_v1(FHString mapFile);
FREEHEROESAPI_EXPORT int      fh_global_get_map_version_v1(void);
FREEHEROESAPI_EXPORT int      fh_global_get_map_width_v1(void);
FREEHEROESAPI_EXPORT int      fh_global_get_map_height_v1(void);
FREEHEROESAPI_EXPORT int      fh_global_get_map_depth_v1(void);
FREEHEROESAPI_EXPORT int      fh_global_get_map_tile_size_v1(void);
FREEHEROESAPI_EXPORT FHResult fh_global_map_derandomize_v1(void);
FREEHEROESAPI_EXPORT FHResult fh_global_map_prepare_render_v1(void);
FREEHEROESAPI_EXPORT FHResult fh_global_map_paint_v1(int x, int y, int z, int width, int height);
FREEHEROESAPI_EXPORT FHBitmap fh_global_get_map_paint_result_v1(void);

typedef struct {
    void (*create)(void);
    void (*destroy)(void);
    FHString (*get_last_error)(void);
    FHString (*get_last_output)(void);

    FHResult (*init)(FHString, FHString);
    FHResult (*convert_lod)(FHString, FHString);
    FHResult (*map_load)(FHString);
    int (*get_map_version)(void);
    int (*get_map_width)(void);
    int (*get_map_height)(void);
    int (*get_map_depth)(void);
    int (*get_map_tile_size)(void);
    FHResult (*map_derandomize)(void);
    FHResult (*map_prepare_render)(void);

    FHResult (*map_paint)(int x, int y, int z, int width, int height);
    FHBitmap (*get_map_paint_result)(void);
} FHApiPointersGlobal;

FREEHEROESAPI_EXPORT FHApiPointersGlobal fh_global_get_api_pointers_v1(void);
typedef FHApiPointersGlobal (*fh_global_get_api_pointers_v1_f)(void);

#ifdef __cplusplus
} // extern "C"
#endif
