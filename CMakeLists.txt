cmake_minimum_required(VERSION 3.30...4.3)
project(UltiJarvis VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(APPLE)
    set(JARVIS_BUNDLE_NAME "Ulti Jarvis.app")
    set(JARVIS_INSTALL_DIR "${JARVIS_BUNDLE_NAME}/Contents/MacOS")
else()
    set(JARVIS_INSTALL_DIR ".")
endif()
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/UltiJarvis"
        CACHE PATH "Where the portable application folder is written" FORCE)
endif()

if(MSVC)
    add_compile_options(/utf-8 /bigobj)
endif()
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(APPLE)
    find_program(JARVIS_BREW NAMES brew HINTS /opt/homebrew/bin /usr/local/bin)
    if(JARVIS_BREW)
        execute_process(COMMAND ${JARVIS_BREW} --prefix
            OUTPUT_VARIABLE JARVIS_BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        foreach(dir "${JARVIS_BREW_PREFIX}" "${JARVIS_BREW_PREFIX}/opt/qt"
                    "${JARVIS_BREW_PREFIX}/opt/boost" "${JARVIS_BREW_PREFIX}/opt/abseil")
            if(IS_DIRECTORY "${dir}")
                list(APPEND CMAKE_PREFIX_PATH "${dir}")
                if(IS_DIRECTORY "${dir}/lib/pkgconfig")
                    set(ENV{PKG_CONFIG_PATH} "${dir}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
                endif()
            endif()
        endforeach()
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Quick QuickControls2 LinguistTools)
find_package(Threads REQUIRED)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.64 REQUIRED COMPONENTS filesystem)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBRTC_APM REQUIRED webrtc-audio-processing-1)

include(FetchContent)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio
    GIT_TAG 0.11.25
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/miniaudio
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(miniaudio)
FetchContent_Declare(
    Lowwi
    GIT_REPOSITORY https://github.com/CLFML/lowwi
    GIT_TAG v3.0
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/Lowwi
    EXCLUDE_FROM_ALL
)
if(WIN32 AND NOT MSVC)
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATH_SUFFIXES onnxruntime)
    find_library(ONNXRUNTIME_IMPLIB NAMES onnxruntime)
    find_file(ONNXRUNTIME_DLL NAMES onnxruntime.dll PATH_SUFFIXES bin)
    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_IMPLIB AND ONNXRUNTIME_DLL)
        add_library(onnxruntime SHARED IMPORTED)
        set_target_properties(onnxruntime PROPERTIES
            IMPORTED_IMPLIB ${ONNXRUNTIME_IMPLIB}
            IMPORTED_LOCATION ${ONNXRUNTIME_DLL}
            INTERFACE_INCLUDE_DIRECTORIES ${ONNXRUNTIME_INCLUDE_DIR})
    endif()
endif()
if(APPLE)
    set(JARVIS_ORT_VERSION 1.18.1)
    FetchContent_Declare(
        onnxruntime_bin
        URL https://github.com/microsoft/onnxruntime/releases/download/v${JARVIS_ORT_VERSION}/onnxruntime-osx-universal2-${JARVIS_ORT_VERSION}.tgz
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/onnxruntime
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(onnxruntime_bin)
    set(JARVIS_ORT_DYLIB
        ${onnxruntime_bin_SOURCE_DIR}/lib/libonnxruntime.${JARVIS_ORT_VERSION}.dylib)
    add_library(onnxruntime SHARED IMPORTED)
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION ${JARVIS_ORT_DYLIB}
        INTERFACE_INCLUDE_DIRECTORIES ${onnxruntime_bin_SOURCE_DIR}/include)
endif()
FetchContent_MakeAvailable(Lowwi)
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp
    GIT_TAG v1.9.1
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/whisper
    EXCLUDE_FROM_ALL
)
find_package(Vulkan QUIET COMPONENTS glslc)
if(Vulkan_FOUND AND Vulkan_glslc_FOUND)
    set(GGML_VULKAN ON CACHE BOOL "" FORCE)
else()
    set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
endif()
if(APPLE)
    set(GGML_METAL_EMBED_LIBRARY ON CACHE BOOL "" FORCE)
endif()
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GGML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(whisper)
if(WIN32)
    set(JARVIS_PLATFORM_SRC app/platform_windows.cpp)
elseif(APPLE)
    set(JARVIS_PLATFORM_SRC app/platform_macos.cpp)
else()
    set(JARVIS_PLATFORM_SRC app/platform_linux.cpp)
endif()
add_executable(UltiJarvis app/main.cpp app/backend.h app/backend.cpp app/audio.cpp app/whisper_wrapper.h app/whisper_wrapper.cpp app/audio_processor.h app/audio_processor.cpp
    app/platform.h app/platform_common.cpp ${JARVIS_PLATFORM_SRC}
    app/apikey.h)
add_executable(JarvisDaemon daemon/main.cpp)
add_executable(JarvisSettings settings/main.cpp settings/backend.h settings/backend.cpp)
set_source_files_properties(app/audio_processor.cpp PROPERTIES COMPILE_OPTIONS "-std=c++17")
qt_add_resources(UltiJarvis "assets"
    PREFIX "/"
    FILES
        app/main.qml
        app/confirm.qml
        images/mic.svg
        fonts/Inter_18pt-Black.ttf
        fonts/PlusJakartaSans-Bold.ttf
)
qt_add_resources(JarvisSettings "settings_assets"
    PREFIX "/"
    BASE settings
    FILES
        settings/settings.qml
)
qt_add_resources(JarvisSettings "settings_fonts"
    PREFIX "/"
    FILES
        fonts/Inter_18pt-Black.ttf
        fonts/PlusJakartaSans-Bold.ttf
)

if(APPLE)
    set(JARVIS_RPATH "@loader_path;@loader_path/../Frameworks")
elseif(UNIX)
    set(JARVIS_RPATH "$ORIGIN")
else()
    set(JARVIS_RPATH "")
endif()
set_target_properties(UltiJarvis PROPERTIES
    OUTPUT_NAME "Ulti Jarvis"
    INSTALL_RPATH "${JARVIS_RPATH}"
    MACOSX_BUNDLE OFF
    WIN32_EXECUTABLE ON
)
set_target_properties(JarvisDaemon PROPERTIES
    OUTPUT_NAME "Ulti-Jarvis-Daemon"
    INSTALL_RPATH "${JARVIS_RPATH}"
    MACOSX_BUNDLE OFF
    WIN32_EXECUTABLE ON
)
set_target_properties(JarvisSettings PROPERTIES
    OUTPUT_NAME "Ulti-Jarvis-Settings"
    INSTALL_RPATH "${JARVIS_RPATH}"
    MACOSX_BUNDLE OFF
    WIN32_EXECUTABLE ON
)
target_include_directories(UltiJarvis PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/whisper/include
    ${CMAKE_SOURCE_DIR}/lib/miniaudio
    ${WEBRTC_APM_INCLUDE_DIRS}
)
target_include_directories(JarvisDaemon PRIVATE ${CMAKE_SOURCE_DIR}/lib/miniaudio)
target_include_directories(JarvisSettings PRIVATE ${CMAKE_SOURCE_DIR}/lib/miniaudio)
target_link_libraries(UltiJarvis PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Quick
    whisper
    ${WEBRTC_APM_LINK_LIBRARIES}
)
target_link_libraries(JarvisDaemon PRIVATE CLFML::lowwi)
if(ONNXRUNTIME_DLL)
    target_compile_definitions(JarvisDaemon PRIVATE
        JARVIS_ONNXRUNTIME_DLL="${ONNXRUNTIME_DLL}")
endif()
target_link_libraries(JarvisSettings PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)
foreach(target IN ITEMS UltiJarvis JarvisDaemon JarvisSettings)
    target_link_libraries(${target} PRIVATE
        Threads::Threads
        Boost::headers
        Boost::filesystem)
    if(WIN32)
        target_link_libraries(${target} PRIVATE ole32 shell32 psapi ws2_32)
    elseif(APPLE)
        target_link_libraries(${target} PRIVATE
            "-framework CoreFoundation"
            "-framework CoreAudio"
            "-framework AudioToolbox")
    else()
        target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS} m)
    endif()
endforeach()
add_custom_command(TARGET JarvisDaemon PRE_LINK
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/stop_daemon.cmake
)
add_custom_command(TARGET JarvisDaemon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/lib/Lowwi/models
        $<TARGET_FILE_DIR:JarvisDaemon>/models
)
set(WHISPER_MODEL_PATH ${CMAKE_SOURCE_DIR}/ggml-small.en-q5_1.bin)
if(NOT EXISTS ${WHISPER_MODEL_PATH})
    message(STATUS "Downloading ggml-small.en-q5_1.bin...")
    file(DOWNLOAD
        https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en-q5_1.bin
        ${WHISPER_MODEL_PATH}
        SHOW_PROGRESS
    )
endif()
add_custom_command(TARGET UltiJarvis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${WHISPER_MODEL_PATH}
        $<TARGET_FILE_DIR:UltiJarvis>/ggml-small.en-q5_1.bin
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/sounds
        $<TARGET_FILE_DIR:UltiJarvis>/sounds
)

file(GLOB_RECURSE LOWWI_ONNXRUNTIME
    ${CMAKE_SOURCE_DIR}/lib/Lowwi/lib/*onnxruntime*.so*
    ${CMAKE_SOURCE_DIR}/lib/Lowwi/lib/*onnxruntime*.dylib
    ${CMAKE_SOURCE_DIR}/lib/Lowwi/lib/*onnxruntime*.dll
)
if(JARVIS_ORT_DYLIB)
    list(APPEND LOWWI_ONNXRUNTIME ${JARVIS_ORT_DYLIB})
endif()
if(LOWWI_ONNXRUNTIME)
    add_custom_command(TARGET JarvisDaemon POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LOWWI_ONNXRUNTIME} $<TARGET_FILE_DIR:JarvisDaemon>
    )
endif()
if(WIN32)
    get_filename_component(JARVIS_RUNTIME_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    install(TARGETS UltiJarvis JarvisDaemon JarvisSettings
        RUNTIME_DEPENDENCY_SET jarvis_runtime
        RUNTIME DESTINATION ${JARVIS_INSTALL_DIR}
    )
    install(RUNTIME_DEPENDENCY_SET jarvis_runtime
        PRE_EXCLUDE_REGEXES "api-ms-win-" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*[Ss]ystem32.*"
        DIRECTORIES ${JARVIS_RUNTIME_DIR}
        DESTINATION ${JARVIS_INSTALL_DIR}
    )
else()
    install(TARGETS UltiJarvis JarvisDaemon JarvisSettings
        RUNTIME DESTINATION ${JARVIS_INSTALL_DIR}
    )
endif()

if(ONNXRUNTIME_DLL)
    install(CODE "
        if(POLICY CMP0207)
            cmake_policy(SET CMP0207 NEW)
        endif()
        file(GET_RUNTIME_DEPENDENCIES
            LIBRARIES \"${ONNXRUNTIME_DLL}\"
            RESOLVED_DEPENDENCIES_VAR _ort_resolved
            UNRESOLVED_DEPENDENCIES_VAR _ort_unresolved
            DIRECTORIES \"${JARVIS_RUNTIME_DIR}\"
            PRE_EXCLUDE_REGEXES \"api-ms-win-\" \"ext-ms-\"
            POST_EXCLUDE_REGEXES \".*[Ss]ystem32.*\"
        )
        file(INSTALL
            DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${JARVIS_INSTALL_DIR}\"
            TYPE SHARED_LIBRARY
            FILES \"${ONNXRUNTIME_DLL}\" \${_ort_resolved}
        )
    ")
endif()
foreach(lib IN ITEMS whisper ggml ggml-base ggml-cpu ggml-vulkan ggml-metal ggml-blas lowwi_lib)
    if(TARGET ${lib})
        set_target_properties(${lib} PROPERTIES INSTALL_RPATH "${JARVIS_RPATH}")
        install(TARGETS ${lib}
            RUNTIME DESTINATION ${JARVIS_INSTALL_DIR}
            LIBRARY DESTINATION ${JARVIS_INSTALL_DIR} NAMELINK_SKIP
            ARCHIVE DESTINATION ${JARVIS_INSTALL_DIR}/lib
        )
    endif()
endforeach()
if(LOWWI_ONNXRUNTIME)
    install(FILES ${LOWWI_ONNXRUNTIME}
        DESTINATION ${JARVIS_INSTALL_DIR})
endif()
if(UNIX AND NOT APPLE)
    set(JARVIS_LOCAL_WEBRTC "")
    foreach(_lib IN LISTS WEBRTC_APM_LINK_LIBRARIES)
        if(_lib MATCHES "^/usr/local/" AND EXISTS "${_lib}")
            list(APPEND JARVIS_LOCAL_WEBRTC "${_lib}")
        endif()
    endforeach()
    if(JARVIS_LOCAL_WEBRTC)
        string(JOIN "\" \"" JARVIS_LOCAL_WEBRTC_QUOTED ${JARVIS_LOCAL_WEBRTC})
        install(CODE "
            file(INSTALL
                DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${JARVIS_INSTALL_DIR}\"
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES \"${JARVIS_LOCAL_WEBRTC_QUOTED}\"
            )
        ")
    endif()
endif()
install(FILES ${WHISPER_MODEL_PATH}
    DESTINATION ${JARVIS_INSTALL_DIR})
install(FILES ${CMAKE_SOURCE_DIR}/images/mic.svg
    DESTINATION ${JARVIS_INSTALL_DIR}
    RENAME ultijarvis.svg)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/lib/Lowwi/models
    DESTINATION ${JARVIS_INSTALL_DIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/sounds
    DESTINATION ${JARVIS_INSTALL_DIR})
if(APPLE)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
        ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/Info.plist
        DESTINATION "${JARVIS_BUNDLE_NAME}/Contents")
endif()

if(WIN32 OR APPLE)
    set(JARVIS_DEPLOY_QT_DEFAULT ON)
else()
    set(JARVIS_DEPLOY_QT_DEFAULT OFF)
endif()
option(JARVIS_DEPLOY_QT "Bundle the Qt runtime into the installed folder"
    ${JARVIS_DEPLOY_QT_DEFAULT})
if(WIN32 AND JARVIS_DEPLOY_QT AND COMMAND qt_generate_deploy_app_script)
    install(CODE "set(QT_DEPLOY_BIN_DIR \"${JARVIS_INSTALL_DIR}\")")
    install(CODE "set(QT_DEPLOY_LIB_DIR \"${JARVIS_INSTALL_DIR}\")")
    install(CODE "set(QT_DEPLOY_PLUGINS_DIR \"plugins\")")
    install(CODE "set(QT_DEPLOY_QML_DIR \"qml\")")
    install(CODE "set(QT_DEPLOY_TRANSLATIONS_DIR \"translations\")")
    if(WIN32)
        set(JARVIS_DEPLOY_TOOL_OPTIONS
            "--qmldir;${CMAKE_CURRENT_SOURCE_DIR};--compiler-runtime")
    elseif(APPLE)
        set(JARVIS_DEPLOY_TOOL_OPTIONS "-qmldir=${CMAKE_CURRENT_SOURCE_DIR}")
    else()
        set(JARVIS_DEPLOY_TOOL_OPTIONS "")
    endif()
    foreach(target IN ITEMS UltiJarvis JarvisSettings)
        qt_generate_deploy_app_script(
            TARGET ${target}
            OUTPUT_SCRIPT JARVIS_DEPLOY_SCRIPT
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS "${JARVIS_DEPLOY_TOOL_OPTIONS}"
        )
        install(SCRIPT ${JARVIS_DEPLOY_SCRIPT})
    endforeach()
    if(WIN32)
        install(CODE "
            if(POLICY CMP0207)
                cmake_policy(SET CMP0207 NEW)
            endif()
            file(GLOB_RECURSE _jarvis_runtime_plugins
                \"\${CMAKE_INSTALL_PREFIX}/plugins/*.dll\"
                \"\${CMAKE_INSTALL_PREFIX}/qml/*.dll\")
            if(_jarvis_runtime_plugins)
                file(GET_RUNTIME_DEPENDENCIES
                    LIBRARIES \${_jarvis_runtime_plugins}
                    RESOLVED_DEPENDENCIES_VAR _jarvis_plugin_deps
                    UNRESOLVED_DEPENDENCIES_VAR _jarvis_plugin_unresolved
                    DIRECTORIES \"${JARVIS_RUNTIME_DIR}\"
                    PRE_EXCLUDE_REGEXES \"api-ms-win-\" \"ext-ms-\"
                    POST_EXCLUDE_REGEXES \".*[Ss]ystem32.*\"
                )
                file(INSTALL
                    DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${JARVIS_INSTALL_DIR}\"
                    TYPE SHARED_LIBRARY
                    FILES \${_jarvis_plugin_deps}
                )
            endif()
        ")
    endif()
endif()

if(APPLE AND JARVIS_DEPLOY_QT)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
        HINTS
            "${JARVIS_BREW_PREFIX}/opt/qt/bin"
            "${JARVIS_BREW_PREFIX}/bin"
    )
    if(MACDEPLOYQT_EXECUTABLE)
        install(CODE "
            set(_bundle \"\${CMAKE_INSTALL_PREFIX}/${JARVIS_BUNDLE_NAME}\")
            set(_macos \"\${_bundle}/Contents/MacOS\")
            file(GLOB _ort \"\${_macos}/libonnxruntime*.dylib\")
            foreach(_lib IN LISTS _ort)
                get_filename_component(_name \"\${_lib}\" NAME)
                execute_process(COMMAND install_name_tool -id \"@rpath/\${_name}\"
                    \"\${_lib}\" ERROR_QUIET)
            endforeach()
            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\" \"\${_bundle}\"
                    \"-executable=\${_macos}/Ulti-Jarvis-Daemon\"
                    \"-executable=\${_macos}/Ulti-Jarvis-Settings\"
                    \"-qmldir=${CMAKE_CURRENT_SOURCE_DIR}\"
                RESULT_VARIABLE _macdeployqt_result
            )
            if(NOT _macdeployqt_result EQUAL 0)
                message(FATAL_ERROR
                    \"macdeployqt failed with \${_macdeployqt_result}\")
            endif()
        ")
    else()
        message(WARNING
            "macdeployqt was not found; the macOS package will link against "
            "Homebrew and will not run on another Mac")
    endif()
endif()

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    find_program(MAKENSIS_EXECUTABLE makensis
        HINTS
            C:/msys64/mingw64/bin
            C:/msys64/mingw32/bin
            "$ENV{ProgramFiles}/NSIS"
            "$ENV{ProgramFiles\(x86\)}/NSIS"
    )
    if(MAKENSIS_EXECUTABLE)
        list(APPEND CPACK_GENERATOR "NSIS")
        set(CPACK_NSIS_EXECUTABLE ${MAKENSIS_EXECUTABLE})
        set(CPACK_PACKAGE_EXECUTABLES "Ulti-Jarvis-Settings;Ulti Jarvis")
        set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
        set(CPACK_NSIS_DISPLAY_NAME "Ulti Jarvis")
        set(CPACK_NSIS_PACKAGE_NAME "Ulti Jarvis")
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
            "ExecShell '' '$INSTDIR\\\\Ulti-Jarvis-Settings.exe'")
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
            "ExecWait 'taskkill /F /IM Ulti-Jarvis-Daemon.exe'")
    else()
        message(STATUS "makensis not found - installer generator disabled")
    endif()
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    file(WRITE ${CMAKE_BINARY_DIR}/packaging/postinst
[=[#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    for u in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $2}'); do
        uid=$(id -u "$u" 2>/dev/null) || continue
        runuser -u "$u" -- env XDG_RUNTIME_DIR=/run/user/$uid \
            DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus \
            /usr/lib/ultijarvis/Ulti-Jarvis-Settings >/dev/null 2>&1 &
    done
fi
exit 0
]=])
    file(CHMOD ${CMAKE_BINARY_DIR}/packaging/postinst
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_BINARY_DIR}/packaging/postinst")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ahmet <ahmetcikko@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "procps, polkitd, xdg-utils, pulseaudio-utils, systemd, libqt6svg6")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libvulkan1")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/lib/ultijarvis")
endif()
set(CPACK_PACKAGE_NAME "UltiJarvis")
set(CPACK_PACKAGE_VENDOR "Ahmet")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Ahmet <ahmetcikko@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "UltiJarvis - Voice Assistant")
set(CPACK_PACKAGE_FILE_NAME "UltiJarvis-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "UltiJarvis")
set(CPACK_VERBATIM_VARIABLES ON)
include(CPack)
