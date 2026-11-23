set(LAMINA_ANDROID_SDK "" CACHE PATH "Android SDK root (ANDROID_HOME)")
set(LAMINA_ANDROID_JDK "" CACHE PATH "JDK root with javac and jar; d8 and apksigner run with it as JAVA_HOME")
set(LAMINA_ANDROID_PLATFORM "" CACHE STRING "SDK platform to compile the Java side against, as named under platforms/")
set(LAMINA_ANDROID_BUILD_TOOLS "" CACHE STRING "SDK build-tools version with aapt2, d8, zipalign and apksigner")
set(LAMINA_ANDROID_MIN_SDK "" CACHE STRING "minSdkVersion of the APK")
set(LAMINA_ANDROID_TARGET_SDK "" CACHE STRING "targetSdkVersion of the APK")
set(LAMINA_ANDROID_SDL_ARCHIVE "" CACHE FILEPATH "SDL source archive of the same version as the linked SDL; its Java sources go into the APK")
set(LAMINA_ANDROID_PAGE_SIZE_KB 16 CACHE STRING "Page size in KiB: ELF page size of libmain.so and zipalign alignment of the stored libraries")
math(EXPR lamina_android_page_size "${LAMINA_ANDROID_PAGE_SIZE_KB} * 1024")
set(LAMINA_ANDROID_PAGE_SIZE "${lamina_android_page_size}" CACHE INTERNAL "Page size in bytes, from LAMINA_ANDROID_PAGE_SIZE_KB")
set(LAMINA_ANDROID_JAVA_RELEASE 8 CACHE STRING "Java source and target level of the Java side")
set(LAMINA_ANDROID_VERSION_CODE 1 CACHE STRING "versionCode of the APK")
set(LAMINA_ANDROID_KEY_ALIAS androiddebugkey CACHE STRING "Key alias in the signing keystore")
set(LAMINA_ANDROID_KEY_PASS "pass:android" CACHE STRING "apksigner password spec for the keystore and key")
if(DEFINED ENV{ANDROID_USER_HOME})
    set(lamina_android_user_home "$ENV{ANDROID_USER_HOME}")
elseif(CMAKE_HOST_WIN32)
    set(lamina_android_user_home "$ENV{USERPROFILE}/.android")
else()
    set(lamina_android_user_home "$ENV{HOME}/.android")
endif()
file(TO_CMAKE_PATH "${lamina_android_user_home}/debug.keystore" lamina_android_default_keystore)
set(LAMINA_ANDROID_KEYSTORE "${lamina_android_default_keystore}" CACHE FILEPATH "Keystore that signs the APK")

set(lamina_android_min_cmake 4.3)
if(CMAKE_VERSION VERSION_LESS lamina_android_min_cmake)
    message(FATAL_ERROR "The APK step stores its entries uncompressed with file(ARCHIVE_CREATE ... FORMAT zip COMPRESSION None), "
        "which needs CMake ${lamina_android_min_cmake} or later; this is ${CMAKE_VERSION}")
endif()

foreach(var LAMINA_ANDROID_SDK LAMINA_ANDROID_JDK LAMINA_ANDROID_PLATFORM LAMINA_ANDROID_BUILD_TOOLS LAMINA_ANDROID_MIN_SDK
        LAMINA_ANDROID_TARGET_SDK LAMINA_ANDROID_SDL_ARCHIVE)
    if(NOT ${var})
        message(FATAL_ERROR "${var} is empty; docs/android.md lists the environment the android presets read")
    endif()
endforeach()
if(NOT EXISTS "${LAMINA_ANDROID_KEYSTORE}")
    message(FATAL_ERROR "No keystore at ${LAMINA_ANDROID_KEYSTORE}; set LAMINA_ANDROID_KEYSTORE")
endif()

set(lamina_android_build_tools_dir "${LAMINA_ANDROID_SDK}/build-tools/${LAMINA_ANDROID_BUILD_TOOLS}")
set(LAMINA_ANDROID_JAR "${LAMINA_ANDROID_SDK}/platforms/${LAMINA_ANDROID_PLATFORM}/android.jar" CACHE INTERNAL "android.jar of LAMINA_ANDROID_PLATFORM")
if(NOT EXISTS "${LAMINA_ANDROID_JAR}")
    message(FATAL_ERROR "No android.jar at ${LAMINA_ANDROID_JAR}")
endif()
function(lamina_android_find_tool var dir)
    find_program(${var} NAMES ${ARGN} PATHS "${dir}" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
endfunction()
lamina_android_find_tool(LAMINA_AAPT2 "${lamina_android_build_tools_dir}" aapt2)
lamina_android_find_tool(LAMINA_D8 "${lamina_android_build_tools_dir}" d8 d8.bat)
lamina_android_find_tool(LAMINA_ZIPALIGN "${lamina_android_build_tools_dir}" zipalign)
lamina_android_find_tool(LAMINA_APKSIGNER "${lamina_android_build_tools_dir}" apksigner apksigner.bat)
lamina_android_find_tool(LAMINA_JAVAC "${LAMINA_ANDROID_JDK}/bin" javac)
lamina_android_find_tool(LAMINA_JAR "${LAMINA_ANDROID_JDK}/bin" jar)
set(lamina_android_triple_x86_64 x86_64-linux-android)
set(lamina_android_triple_arm64-v8a aarch64-linux-android)
set(lamina_android_triple "${lamina_android_triple_${ANDROID_ABI}}")
if(NOT lamina_android_triple)
    message(FATAL_ERROR "No library triple for ANDROID_ABI ${ANDROID_ABI}; add one to cmake/LaminaAndroid.cmake")
endif()
find_file(LAMINA_ANDROID_CXX_SHARED libc++_shared.so PATHS "${CMAKE_SYSROOT}/usr/lib/${lamina_android_triple}"
    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)

set(lamina_android_sdl_dir "${CMAKE_BINARY_DIR}/sdl-android")
if(NOT LAMINA_ANDROID_SDL_EXTRACTED STREQUAL LAMINA_ANDROID_SDL_ARCHIVE)
    file(REMOVE_RECURSE "${lamina_android_sdl_dir}")
    file(ARCHIVE_EXTRACT INPUT "${LAMINA_ANDROID_SDL_ARCHIVE}" DESTINATION "${lamina_android_sdl_dir}"
        PATTERNS "*/android-project/app/src/main/java/*")
    set(LAMINA_ANDROID_SDL_EXTRACTED "${LAMINA_ANDROID_SDL_ARCHIVE}" CACHE INTERNAL "")
endif()
file(GLOB_RECURSE LAMINA_ANDROID_SDL_JAVA "${lamina_android_sdl_dir}/*/android-project/app/src/main/java/org/libsdl/app/*.java")
list(FILTER LAMINA_ANDROID_SDL_JAVA INCLUDE REGEX "/SDLActivity\\.java$")
if(NOT LAMINA_ANDROID_SDL_JAVA)
    message(FATAL_ERROR "${LAMINA_ANDROID_SDL_ARCHIVE} has no android-project/.../SDLActivity.java")
endif()
get_filename_component(lamina_android_sdl_java_dir "${LAMINA_ANDROID_SDL_JAVA}" DIRECTORY)
file(READ "${LAMINA_ANDROID_SDL_JAVA}" lamina_sdl_java_source)
set(lamina_sdl_java_version "")
foreach(part MAJOR MINOR MICRO)
    string(REGEX MATCH "SDL_${part}_VERSION = ([0-9]+)" lamina_sdl_java_match "${lamina_sdl_java_source}")
    list(APPEND lamina_sdl_java_version "${CMAKE_MATCH_1}")
endforeach()
list(JOIN lamina_sdl_java_version "." lamina_sdl_java_version)
if(NOT lamina_sdl_java_version VERSION_EQUAL SDL3_VERSION)
    message(FATAL_ERROR "The SDL Java sources in ${LAMINA_ANDROID_SDL_ARCHIVE} are ${lamina_sdl_java_version}, but the linked SDL is ${SDL3_VERSION}; they must match")
endif()
file(GLOB lamina_android_sdl_java_files "${lamina_android_sdl_java_dir}/*.java")
set(LAMINA_ANDROID_SDL_JAVA_FILES "${lamina_android_sdl_java_files}" CACHE INTERNAL "SDL Java sources that go into every APK")

function(lamina_android_apk target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "PACKAGE;LABEL;OUTPUT_NAME;MANIFEST;RES" "EXTRA_RES")
    set(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    if(NOT arg_MANIFEST)
        set(arg_MANIFEST "${root}/android/AndroidManifest.xml.in")
    endif()
    if(NOT arg_RES)
        set(arg_RES "${root}/android/res")
    endif()
    set(dir "${CMAKE_CURRENT_BINARY_DIR}/android/${target}")
    set(LAMINA_APK_PACKAGE "${arg_PACKAGE}")
    set(LAMINA_APK_LABEL "${arg_LABEL}")
    set(LAMINA_APK_LIBRARY "$<TARGET_FILE_BASE_NAME:${target}>")
    configure_file("${arg_MANIFEST}" "${dir}/AndroidManifest.xml" @ONLY)
    configure_file("${root}/android/LaminaActivity.java.in" "${dir}/LaminaActivity.java.in" @ONLY)
    file(GENERATE OUTPUT "${dir}/java/LaminaActivity.java" INPUT "${dir}/LaminaActivity.java.in")

    set(java ${LAMINA_ANDROID_SDL_JAVA_FILES} "${dir}/java/LaminaActivity.java")
    add_custom_command(OUTPUT "${dir}/classes.dex"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${dir}/classes" "${dir}/dex"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}/classes" "${dir}/dex"
        COMMAND "${LAMINA_JAVAC}" -J-Duser.language=en -source ${LAMINA_ANDROID_JAVA_RELEASE} -target ${LAMINA_ANDROID_JAVA_RELEASE} -Xlint:-options
            -nowarn -encoding UTF-8 -bootclasspath "${LAMINA_ANDROID_JAR}" -d "${dir}/classes" ${java}
        COMMAND "${LAMINA_JAR}" --create --file "${dir}/classes.jar" -C "${dir}/classes" .
        COMMAND "${CMAKE_COMMAND}" -E env "JAVA_HOME=${LAMINA_ANDROID_JDK}"
            "${LAMINA_D8}" --release --min-api ${LAMINA_ANDROID_MIN_SDK} --lib "${LAMINA_ANDROID_JAR}" --output "${dir}/dex" "${dir}/classes.jar"
        COMMAND "${CMAKE_COMMAND}" -E copy "${dir}/dex/classes.dex" "${dir}/classes.dex"
        DEPENDS ${java}
        VERBATIM)

    set(compiled "")
    set(index 0)
    foreach(res_dir IN LISTS arg_RES arg_EXTRA_RES)
        get_filename_component(res_dir "${res_dir}" ABSOLUTE)
        file(GLOB_RECURSE resources CONFIGURE_DEPENDS "${res_dir}/*")
        set(zip "${dir}/resources-${index}.zip")
        add_custom_command(OUTPUT "${zip}"
            COMMAND "${LAMINA_AAPT2}" compile --dir "${res_dir}" -o "${zip}"
            DEPENDS ${resources}
            VERBATIM)
        list(APPEND compiled "${zip}")
        math(EXPR index "${index} + 1")
    endforeach()
    add_custom_command(OUTPUT "${dir}/base.apk"
        COMMAND "${LAMINA_AAPT2}" link -o "${dir}/base.apk" --manifest "${dir}/AndroidManifest.xml" -I "${LAMINA_ANDROID_JAR}"
            --min-sdk-version ${LAMINA_ANDROID_MIN_SDK} --target-sdk-version ${LAMINA_ANDROID_TARGET_SDK}
            --version-code ${LAMINA_ANDROID_VERSION_CODE} --version-name ${PROJECT_VERSION} --debug-mode ${compiled}
        DEPENDS "${dir}/AndroidManifest.xml" ${compiled}
        VERBATIM)

    set(unaligned "${dir}/unaligned.apk")
    set(aligned "${dir}/aligned.apk")
    set(apk "${CMAKE_CURRENT_BINARY_DIR}/${arg_OUTPUT_NAME}-${ANDROID_ABI}.apk")
    add_custom_command(OUTPUT "${unaligned}"
        COMMAND "${CMAKE_COMMAND}" "-DBASE_APK=${dir}/base.apk" "-DDEX=${dir}/classes.dex" "-DABI=${ANDROID_ABI}"
            "-DLIBRARIES=$<TARGET_FILE:${target}>;${LAMINA_ANDROID_CXX_SHARED}" "-DSTRIP=${CMAKE_STRIP}"
            "-DSTAGING=${dir}/staging" "-DOUTPUT=${unaligned}" -P "${root}/cmake/LaminaAndroidAssemble.cmake"
        DEPENDS "${dir}/base.apk" "${dir}/classes.dex" ${target} "${root}/cmake/LaminaAndroidAssemble.cmake"
        VERBATIM)
    add_custom_command(OUTPUT "${apk}"
        COMMAND "${LAMINA_ZIPALIGN}" -P ${LAMINA_ANDROID_PAGE_SIZE_KB} -f 4 "${unaligned}" "${aligned}"
        COMMAND "${CMAKE_COMMAND}" -E env "JAVA_HOME=${LAMINA_ANDROID_JDK}"
            "${LAMINA_APKSIGNER}" sign --ks "${LAMINA_ANDROID_KEYSTORE}" --ks-key-alias ${LAMINA_ANDROID_KEY_ALIAS}
            --ks-pass ${LAMINA_ANDROID_KEY_PASS} --key-pass ${LAMINA_ANDROID_KEY_PASS} --out "${apk}" "${aligned}"
        DEPENDS "${unaligned}"
        BYPRODUCTS "${aligned}" "${apk}.idsig"
        VERBATIM)
    add_custom_target(${target}_apk ALL DEPENDS "${apk}")
endfunction()
