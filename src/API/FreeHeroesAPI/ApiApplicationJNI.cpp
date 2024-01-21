#include "ApiApplicationC.h"
#include "ApiApplication.hpp"
#include "jni.h"

#define APP_PREFIX com_example_freeheroeslwp
#define DEFAULT_ACTIVITY JniActivity
#define CONCAT1(prefix, class, function) CONCAT2(prefix, class, function)
#define CONCAT2(prefix, class, function) Java_##prefix##_##class##_##function
#define JNI_ACTIVITY_FN(function) CONCAT1(APP_PREFIX, DEFAULT_ACTIVITY, function)

const char* jstringToChar(JNIEnv* env, jstring str)
{
    return (*env).GetStringUTFChars(str, 0);
}

extern "C" JNIEXPORT jlong JNICALL
JNI_ACTIVITY_FN(createV1)(JNIEnv* env, jobject)
{
    return reinterpret_cast<intptr_t>(fh_create_v1());
}

extern "C" JNIEXPORT void JNICALL
JNI_ACTIVITY_FN(destroyV1)(JNIEnv* env, jobject, jlong fhAppHandle)
{
    return fh_destroy_v1((FHAppHandle) fhAppHandle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(initV1)(JNIEnv* env, jobject, jlong fhAppHandle, jstring mainResourcePath, jstring userResourcePath)
{
    return fh_init_v1((FHAppHandle) fhAppHandle, jstringToChar(env, mainResourcePath), jstringToChar(env, userResourcePath));
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(reinitV1)(JNIEnv* env, jobject, jlong fhAppHandle)
{
    return fh_reinit_v1((FHAppHandle) fhAppHandle);
}

extern "C" JNIEXPORT jstring JNICALL
JNI_ACTIVITY_FN(getLastErrorV1)(JNIEnv* env, jobject, jlong fhAppHandle)
{
    return (*env).NewStringUTF(fh_get_last_error_v1((FHAppHandle) fhAppHandle));
}

extern "C" JNIEXPORT jstring JNICALL
JNI_ACTIVITY_FN(getLastOutputV1)(JNIEnv* env, jobject, jlong fhAppHandle)
{
    return (*env).NewStringUTF(fh_get_last_output_v1((FHAppHandle) fhAppHandle));
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(convertLodV1)(JNIEnv* env, jobject, jlong fhAppHandle, jstring lodPath)
{
    return fh_convert_lod_v1((FHAppHandle) fhAppHandle, jstringToChar(env, lodPath));
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(mapLoadV1)(JNIEnv* env, jobject, jlong fhAppHandle, jstring mapPath)
{
    return fh_map_load_v1((FHAppHandle) fhAppHandle, jstringToChar(env, mapPath));
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(getMapVersionV1)(JNIEnv* env, jobject, jlong fh_app_handle)
{
    return fh_get_map_version_v1((FHAppHandle) fh_app_handle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(getMapWidthV1)(JNIEnv* env, jobject, jlong fh_app_handle)
{
    return fh_get_map_width_v1((FHAppHandle) fh_app_handle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(getMapHeightV1)(JNIEnv* env, jobject, jlong fh_app_handle)
{
    return fh_get_map_height_v1((FHAppHandle) fh_app_handle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(getMapDepthV1)(JNIEnv* env, jobject, jlong fh_app_handle)
{
    return fh_get_map_depth_v1((FHAppHandle) fh_app_handle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(getMapTileSizeV1)(JNIEnv* env, jobject, jlong fh_app_handle)
{
    return fh_get_map_tile_size_v1((FHAppHandle) fh_app_handle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(mapDerandomizeV1)(JNIEnv* env, jobject, jlong fhAppHandle)
{
    return fh_map_derandomize_v1((FHAppHandle) fhAppHandle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(prepareRenderV1)(JNIEnv* env, jobject, jlong fhAppHandle)
{
    return fh_map_prepare_render_v1((FHAppHandle) fhAppHandle);
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(paintV1)(JNIEnv* env, jobject, jlong fhAppHandle)
{
    return fh_map_paint_v1((FHAppHandle) fhAppHandle);
}

extern "C" JNIEXPORT jbyteArray JNICALL
JNI_ACTIVITY_FN(getMapPaintResultV1)(JNIEnv* env, jobject, jlong fhAppHandle, jint visibleArea)
{
    int length = visibleArea * 4;

    FHBitmap   bytes      = fh_get_map_paint_result_v1((FHAppHandle) fhAppHandle);
    jbyteArray byte_array = env->NewByteArray(length);
    auto       jbytes     = reinterpret_cast<const jbyte*>(bytes);
    env->SetByteArrayRegion(byte_array, 0, length, jbytes);

    return byte_array;
}

extern "C" JNIEXPORT jint JNICALL
JNI_ACTIVITY_FN(setMapRenderWindowV1)(JNIEnv* env, jobject, jlong fhAppHandle, jint x, jint y, jint z, jint width, jint height)
{
    return fh_set_map_render_window_v1((FHAppHandle) fhAppHandle, (int) x, (int) y, (int) z, (int) width, (int) height);
}
