#include "lamina_sdl3/system_ui.hpp"

#include <string>

#include <jni.h>

#include <SDL3/SDL.h>

#include "lamina_sdl3/report.hpp"

namespace lamina::sdl3::detail {

namespace {

constexpr const char* dark_system_bars_method = "setDarkSystemBars";
constexpr const char* dark_system_bars_signature = "(Z)V";

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

}
