#include "lamina_sdl3/system_ui.hpp"

#include <array>
#include <string>

#include <jni.h>

#include <SDL3/SDL.h>

#include "lamina_sdl3/report.hpp"

namespace lamina::sdl3::detail {

namespace {

constexpr const char* dark_system_bars_method = "setDarkSystemBars";
constexpr const char* dark_system_bars_signature = "(Z)V";
constexpr const char* hidden_system_bars_method = "setSystemBarsHidden";
constexpr const char* hidden_system_bars_signature = "(Z)V";
constexpr const char* window_insets_method = "getWindowInsets";
constexpr const char* window_insets_signature = "()[I";
constexpr jsize inset_edges = 4;
constexpr jsize content_offset = 0;
constexpr jsize gestures_offset = content_offset + inset_edges;
constexpr jsize inset_values = gestures_offset + inset_edges;

layout::Insets edges_at(const std::array<jint, inset_values>& values, jsize offset) {
    return {static_cast<double>(values[offset]), static_cast<double>(values[offset + 1]), static_cast<double>(values[offset + 2]),
            static_cast<double>(values[offset + 3])};
}

bool call_activity(const char* method, const char* signature, bool argument) {
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) {
        return false;
    }
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!activity) {
        return false;
    }
    jclass type = env->GetObjectClass(activity);
    const jmethodID id = env->GetMethodID(type, method, signature);
    bool called = false;
    if (id) {
        env->CallVoidMethod(activity, id, argument ? JNI_TRUE : JNI_FALSE);
        called = !env->ExceptionCheck();
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
    return called;
}

}

bool is_system_back(SDL_Keycode key) {
    return key == SDLK_AC_BACK;
}

void system_back() {
    SDL_SendAndroidBackButton();
}

void set_dark_system_bars(bool dark) {
    if (!call_activity(dark_system_bars_method, dark_system_bars_signature, dark)) {
        report_error(std::string("lamina: the activity has no ") + dark_system_bars_method + dark_system_bars_signature +
                     "; the system bar icons keep their color");
    }
}

void set_system_bars_hidden(bool hidden) {
    if (!call_activity(hidden_system_bars_method, hidden_system_bars_signature, hidden)) {
        report_error(std::string("lamina: the activity has no ") + hidden_system_bars_method + hidden_system_bars_signature +
                     "; the system bars stay shown");
    }
}

std::optional<SystemInsets> system_insets_pixels() {
    static bool missing_reported = false;
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) {
        return std::nullopt;
    }
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!activity) {
        return std::nullopt;
    }
    jclass type = env->GetObjectClass(activity);
    const jmethodID id = env->GetMethodID(type, window_insets_method, window_insets_signature);
    std::optional<SystemInsets> insets;
    if (id) {
        auto values = static_cast<jintArray>(env->CallObjectMethod(activity, id));
        if (!env->ExceptionCheck() && values && env->GetArrayLength(values) == inset_values) {
            std::array<jint, inset_values> read{};
            env->GetIntArrayRegion(values, 0, inset_values, read.data());
            insets = SystemInsets{edges_at(read, content_offset), edges_at(read, gestures_offset)};
        }
        if (values) {
            env->DeleteLocalRef(values);
        }
    } else if (!missing_reported) {
        missing_reported = true;
        report_error(std::string("lamina: the activity has no ") + window_insets_method + window_insets_signature +
                     "; the layout keeps out of SDL's safe area, which includes the gesture insets");
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
    return insets;
}

}
