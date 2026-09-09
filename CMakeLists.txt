cmake_minimum_required(VERSION 3.30...4.3)
if(CMAKE_HOST_APPLE)

    set(JARVIS_MACOS_MIN "14.0" CACHE STRING
        "Oldest macOS release the build targets")
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "${JARVIS_MACOS_MIN}")
    endif()
endif()
project(UltiJarvis VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(LINUX)
    add_compile_options(
        "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=."
        "-ffile-prefix-map=${CMAKE_BINARY_DIR}=.")
endif()

if(APPLE)
    set(JARVIS_BUNDLE_NAME "Ulti Jarvis.app")
    set(JARVIS_INSTALL_DIR "${JARVIS_BUNDLE_NAME}/Contents/MacOS")

    set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "UltiJarvis")
elseif(LINUX)
    set(JARVIS_INSTALL_DIR "lib/ultijarvis")
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

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/app/apikey.h)
    configure_file(${CMAKE_SOURCE_DIR}/app/apikey.h.example
        ${CMAKE_SOURCE_DIR}/app/apikey.h COPYONLY)
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
set(JARVIS_ORT_VERSION 1.18.1)
if(WIN32)
    FetchContent_Declare(
        onnxruntime_bin
        URL https://github.com/microsoft/onnxruntime/releases/download/v${JARVIS_ORT_VERSION}/onnxruntime-win-x64-${JARVIS_ORT_VERSION}.zip
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/onnxruntime
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(onnxruntime_bin)
    set(ONNXRUNTIME_DLL ${onnxruntime_bin_SOURCE_DIR}/lib/onnxruntime.dll)
    set(ONNXRUNTIME_IMPLIB ${onnxruntime_bin_SOURCE_DIR}/lib/onnxruntime.lib)
    add_library(onnxruntime SHARED IMPORTED)
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_IMPLIB ${ONNXRUNTIME_IMPLIB}
        IMPORTED_LOCATION ${ONNXRUNTIME_DLL}
        INTERFACE_INCLUDE_DIRECTORIES ${onnxruntime_bin_SOURCE_DIR}/include)
endif()
if(APPLE)
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
set(GGML_NATIVE OFF CACHE BOOL "" FORCE)
set(GGML_AVX OFF CACHE BOOL "" FORCE)
set(GGML_AVX2 OFF CACHE BOOL "" FORCE)
set(GGML_AVX512 OFF CACHE BOOL "" FORCE)
set(GGML_FMA OFF CACHE BOOL "" FORCE)
set(GGML_F16C OFF CACHE BOOL "" FORCE)
set(GGML_BMI2 OFF CACHE BOOL "" FORCE)
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
if(APPLE)

    function(jarvis_force_include target dir src header)
        if(NOT TARGET ${target})
            return()
        endif()
        set_source_files_properties(${dir}/${src} DIRECTORY ${dir}
            PROPERTIES COMPILE_OPTIONS "-include;${header}")
    endfunction()
    jarvis_force_include(ggml-base ${CMAKE_SOURCE_DIR}/lib/whisper/ggml/src
        gguf.cpp cerrno)
    jarvis_force_include(ggml-metal
        ${CMAKE_SOURCE_DIR}/lib/whisper/ggml/src/ggml-metal
        ggml-metal.cpp memory)
endif()
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
if(WIN32)
    enable_language(RC)
    qt_add_resources(UltiJarvis "windows_icon"
        PREFIX "/"
        FILES
            images/ultijarvis.svg
    )
    target_sources(UltiJarvis PRIVATE
        ${CMAKE_SOURCE_DIR}/images/windows_icon.rc)
    qt_add_resources(JarvisSettings "windows_icon_settings"
        PREFIX "/"
        FILES
            images/ultijarvis.svg
    )
    target_sources(JarvisSettings PRIVATE
        ${CMAKE_SOURCE_DIR}/images/windows_icon.rc)
endif()
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
if(LINUX)
    set(JARVIS_LINUX_SYSTEM_LIBS
        "^ld-linux.*" "^libc[.]so.*" "^libm[.]so.*" "^libdl[.]so.*"
        "^librt[.]so.*" "^libpthread[.]so.*" "^libresolv[.]so.*"
        "^libgcc_s[.]so.*" "^libstdc[+][+][.]so.*"
        "^libEGL[.]so.*" "^libGL[.]so.*" "^libGLX[.]so.*"
        "^libGLdispatch[.]so.*" "^libGLESv2[.]so.*" "^libOpenGL[.]so.*"
        "^libvulkan[.]so.*" "^libdrm[.]so.*" "^libgbm[.]so.*"
        "^libX11.*" "^libxcb[.]so.*" "^libXau[.]so.*" "^libXdmcp[.]so.*"
        "^libXext[.]so.*" "^libXrender[.]so.*" "^libxkbcommon[.]so.*"
        "^libwayland.*" "^libselinux[.]so.*" "^libsystemd[.]so.*"
        "^libudev[.]so.*" "^libdbus-1[.]so.*"
    )
    foreach(jarvis_target IN ITEMS UltiJarvis JarvisDaemon JarvisSettings)
        target_link_options(${jarvis_target} PRIVATE "LINKER:--disable-new-dtags")
    endforeach()
endif()
target_include_directories(UltiJarvis PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/whisper/include
    ${CMAKE_SOURCE_DIR}/lib/miniaudio
    ${WEBRTC_APM_INCLUDE_DIRS}
)
target_include_directories(JarvisDaemon PRIVATE ${CMAKE_SOURCE_DIR}/lib/miniaudio)
target_include_directories(JarvisSettings PRIVATE ${CMAKE_SOURCE_DIR}/lib/miniaudio
    ${CMAKE_SOURCE_DIR}/app)
target_link_libraries(UltiJarvis PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Quick
    whisper
    ${WEBRTC_APM_LINK_LIBRARIES}
)
target_link_libraries(JarvisDaemon PRIVATE CLFML::lowwi)
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
elseif(LINUX)
    install(TARGETS UltiJarvis JarvisDaemon JarvisSettings
        RUNTIME_DEPENDENCY_SET jarvis_runtime
        RUNTIME DESTINATION ${JARVIS_INSTALL_DIR}
    )
    install(RUNTIME_DEPENDENCY_SET jarvis_runtime
        PRE_EXCLUDE_REGEXES ${JARVIS_LINUX_SYSTEM_LIBS}
        POST_EXCLUDE_REGEXES "^/lib/ld-linux" "^/usr/lib/ld-linux"
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
if(WIN32)
    set(JARVIS_MSVC_RUNTIME "")
    set(JARVIS_SYSTEM_RUNTIME
        MSVCP140.dll MSVCP140_1.dll VCRUNTIME140.dll VCRUNTIME140_1.dll)
    if(GGML_VULKAN)
        list(APPEND JARVIS_SYSTEM_RUNTIME vulkan-1.dll)
    endif()
    foreach(_crt IN LISTS JARVIS_SYSTEM_RUNTIME)
        file(TO_CMAKE_PATH "$ENV{SystemRoot}/System32/${_crt}" _crt_path)
        if(EXISTS "${_crt_path}")
            list(APPEND JARVIS_MSVC_RUNTIME "${_crt_path}")
        else()
            message(FATAL_ERROR
                "${_crt} was not found in System32. It is not part of Windows "
                "and has to ship with the application, or the program will not "
                "start on a machine that does not already have it.")
        endif()
    endforeach()
    install(FILES ${JARVIS_MSVC_RUNTIME}
        DESTINATION ${JARVIS_INSTALL_DIR})
    install(FILES ${JARVIS_MSVC_RUNTIME}
        DESTINATION ${JARVIS_INSTALL_DIR}/restore)
endif()

if(ONNXRUNTIME_DLL)
    install(FILES ${ONNXRUNTIME_DLL}
        DESTINATION ${JARVIS_INSTALL_DIR}/restore)
endif()
install(FILES
        ${CMAKE_SOURCE_DIR}/lib/Lowwi/models/melspectrogram.onnx
        ${CMAKE_SOURCE_DIR}/lib/Lowwi/models/embedding_model.onnx
    DESTINATION ${JARVIS_INSTALL_DIR}/restore/models)
install(FILES
        ${CMAKE_SOURCE_DIR}/lib/Lowwi/models/example_wakewords/hey_jarvis.onnx
    DESTINATION ${JARVIS_INSTALL_DIR}/restore/models/example_wakewords)
install(FILES ${CMAKE_SOURCE_DIR}/images/mic.svg
    DESTINATION ${JARVIS_INSTALL_DIR}
    RENAME ultijarvis.svg)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/lib/Lowwi/models
    DESTINATION ${JARVIS_INSTALL_DIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/sounds
    DESTINATION ${JARVIS_INSTALL_DIR})
if(APPLE)

    configure_file(${CMAKE_SOURCE_DIR}/cmake/macos_layout.cmake.in
        ${CMAKE_BINARY_DIR}/macos_layout.cmake @ONLY)
    install(SCRIPT ${CMAKE_BINARY_DIR}/macos_layout.cmake)
endif()

if(APPLE)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
        ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/Info.plist
        DESTINATION "${JARVIS_BUNDLE_NAME}/Contents")
    install(FILES ${CMAKE_SOURCE_DIR}/images/UltiJarvis.icns
        DESTINATION "${JARVIS_BUNDLE_NAME}/Contents/Resources")
endif()

if(WIN32 OR APPLE)
    set(JARVIS_DEPLOY_QT_DEFAULT ON)
elseif(LINUX)
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

if(LINUX AND JARVIS_DEPLOY_QT)
    get_target_property(JARVIS_QMAKE Qt6::qmake IMPORTED_LOCATION)
    execute_process(COMMAND ${JARVIS_QMAKE} -query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE JARVIS_QT_PLUGIN_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${JARVIS_QMAKE} -query QT_INSTALL_QML
        OUTPUT_VARIABLE JARVIS_QT_QML_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    foreach(jarvis_platform_plugin IN ITEMS libqxcb libqoffscreen libqminimal
            libqwayland-generic libqwayland-egl)
        if(EXISTS "${JARVIS_QT_PLUGIN_DIR}/platforms/${jarvis_platform_plugin}.so")
            install(FILES
                "${JARVIS_QT_PLUGIN_DIR}/platforms/${jarvis_platform_plugin}.so"
                DESTINATION "${JARVIS_INSTALL_DIR}/plugins/platforms")
        endif()
    endforeach()
    foreach(jarvis_plugin_dir IN ITEMS xcbglintegrations imageformats
            iconengines tls networkinformation wayland-shell-integration
            wayland-decoration-client wayland-graphics-integration-client)
        if(EXISTS "${JARVIS_QT_PLUGIN_DIR}/${jarvis_plugin_dir}")
            install(DIRECTORY "${JARVIS_QT_PLUGIN_DIR}/${jarvis_plugin_dir}"
                DESTINATION "${JARVIS_INSTALL_DIR}/plugins"
                FILES_MATCHING PATTERN "*.so")
        endif()
    endforeach()
    foreach(jarvis_qml_dir IN ITEMS QtCore QtQml QtQuick)
        if(EXISTS "${JARVIS_QT_QML_DIR}/${jarvis_qml_dir}")
            install(DIRECTORY "${JARVIS_QT_QML_DIR}/${jarvis_qml_dir}"
                DESTINATION "${JARVIS_INSTALL_DIR}/qml"
                PATTERN "Pdf" EXCLUDE
                PATTERN "Scene2D" EXCLUDE
                PATTERN "Scene3D" EXCLUDE
                PATTERN "VirtualKeyboard" EXCLUDE
                PATTERN "Particles" EXCLUDE
                PATTERN "tooling" EXCLUDE)
        endif()
    endforeach()
    file(WRITE ${CMAKE_BINARY_DIR}/packaging/qt.conf
"[Paths]
Prefix = .
Plugins = plugins
Imports = qml
Qml2Imports = qml
")
    install(FILES ${CMAKE_BINARY_DIR}/packaging/qt.conf
        DESTINATION ${JARVIS_INSTALL_DIR})
    configure_file(${CMAKE_SOURCE_DIR}/cmake/linux_plugin_deps.cmake.in
        ${CMAKE_BINARY_DIR}/linux_plugin_deps.cmake @ONLY)
    install(SCRIPT ${CMAKE_BINARY_DIR}/linux_plugin_deps.cmake)
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
                    \"-executable=\${_macos}/Ulti Jarvis\"
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

if(APPLE)
    find_program(JARVIS_VTOOL vtool)
    find_program(JARVIS_OTOOL otool)
    find_program(JARVIS_INSTALL_NAME_TOOL install_name_tool)
    if(JARVIS_VTOOL AND JARVIS_OTOOL AND JARVIS_INSTALL_NAME_TOOL)
        configure_file(${CMAKE_SOURCE_DIR}/cmake/macos_binaries.cmake.in
            ${CMAKE_BINARY_DIR}/macos_binaries.cmake @ONLY)
        install(SCRIPT ${CMAKE_BINARY_DIR}/macos_binaries.cmake)
    else()
        message(WARNING
            "vtool was not found; bundled libraries will keep the build "
            "machine's macOS minimum")
    endif()
endif()

if(APPLE)

    set(JARVIS_CODESIGN_IDENTITY "-" CACHE STRING
        "codesign identity for the macOS bundle, or - for an ad-hoc signature")
    set(JARVIS_NOTARIZE_PROFILE "" CACHE STRING
        "notarytool keychain profile; set with a Developer ID to notarise")
    find_program(JARVIS_CODESIGN codesign)
    if(JARVIS_CODESIGN)
        configure_file(${CMAKE_SOURCE_DIR}/cmake/codesign.cmake.in
            ${CMAKE_BINARY_DIR}/codesign.cmake @ONLY)

        install(SCRIPT ${CMAKE_BINARY_DIR}/codesign.cmake)
    else()
        message(WARNING
            "codesign was not found; the bundle will keep the ad-hoc signature "
            "the linker wrote, which Gatekeeper rejects")
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

    set(CPACK_GENERATOR "productbuild")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/Applications")
    set(CPACK_COMPONENTS_ALL UltiJarvis)
    set(CPACK_PRODUCTBUILD_IDENTIFIER "com.ultijarvis.UltiJarvis")

    file(WRITE ${CMAKE_BINARY_DIR}/packaging/postinstall
[=[#!/bin/sh

uid=$(/usr/bin/stat -f %u /dev/console 2>/dev/null) || exit 0
[ -n "$uid" ] && [ "$uid" -gt 0 ] || exit 0

tmp=$(/usr/bin/sudo -u "#$uid" /usr/bin/getconf DARWIN_USER_TEMP_DIR 2>/dev/null)
if [ -n "$tmp" ]; then
    TMPDIR="$tmp"
    export TMPDIR
else
    unset TMPDIR
fi

/bin/launchctl asuser "$uid" /usr/bin/sudo -u "#$uid" \
    /usr/bin/open "/Applications/Ulti Jarvis.app" || true
exit 0
]=])
    file(CHMOD ${CMAKE_BINARY_DIR}/packaging/postinstall
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set(CPACK_POSTFLIGHT_ULTIJARVIS_SCRIPT
        ${CMAKE_BINARY_DIR}/packaging/postinstall)

    find_program(JARVIS_PKGUTIL pkgutil)
    if(JARVIS_PKGUTIL)
        configure_file(${CMAKE_SOURCE_DIR}/cmake/macos_pkg.cmake.in
            ${CMAKE_BINARY_DIR}/macos_pkg.cmake @ONLY)
        set(CPACK_POST_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/macos_pkg.cmake)
    else()
        message(WARNING
            "pkgutil was not found; the installer will ask for Rosetta")
    endif()
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
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "procps, polkitd, xdg-utils, pulseaudio-utils, systemd, libc6, libstdc++6, libgcc-s1, libgl1, libegl1, libglx0, libopengl0, libx11-6, libx11-xcb1, libxcb1, libxkbcommon0, libdbus-1-3, libselinux1, libwayland-client0, libwayland-cursor0, libwayland-egl1")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libvulkan1")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()
if(LINUX)
    find_program(JARVIS_RPMBUILD rpmbuild)
    if(JARVIS_RPMBUILD)
        list(APPEND CPACK_GENERATOR "RPM")
    else()
        message(STATUS "rpmbuild not found - RPM generator disabled")
    endif()
    file(WRITE ${CMAKE_BINARY_DIR}/packaging/rpm_post
[=[#!/bin/sh
for u in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $2}'); do
    uid=$(id -u "$u" 2>/dev/null) || continue
    runuser -u "$u" -- env XDG_RUNTIME_DIR=/run/user/$uid \
        DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus \
        /usr/lib/ultijarvis/Ulti-Jarvis-Settings >/dev/null 2>&1 &
done
exit 0
]=])
    file(CHMOD ${CMAKE_BINARY_DIR}/packaging/rpm_post
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
        ${CMAKE_BINARY_DIR}/packaging/rpm_post)
    set(CPACK_RPM_PACKAGE_AUTOREQ OFF)
    set(CPACK_RPM_PACKAGE_AUTOPROV OFF)
    set(CPACK_RPM_PACKAGE_NAME "ultijarvis")
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-only")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
    set(CPACK_RPM_PACKAGE_REQUIRES "procps-ng, polkit, xdg-utils, pulseaudio-utils, systemd, glibc, libstdc++, libgcc, mesa-libGL, mesa-libEGL, libglvnd-glx, libglvnd-opengl, libX11, libX11-xcb, libxcb, libxkbcommon, dbus-libs, libselinux, libwayland-client, libwayland-cursor, libwayland-egl")
    set(CPACK_RPM_SPEC_MORE_DEFINE
        "%global __os_install_post %{nil}
%global _build_id_links none
%global __requires_exclude_from ^/usr/lib/ultijarvis/.*$
%global __provides_exclude_from ^/usr/lib/ultijarvis/.*$")
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        "/usr/lib;/usr/share;/usr/share/applications;/usr/share/icons;/usr/share/icons/hicolor;/usr/share/icons/hicolor/scalable;/usr/share/icons/hicolor/scalable/apps")
    file(WRITE ${CMAKE_BINARY_DIR}/packaging/ultijarvis.desktop
"[Desktop Entry]
Type=Application
Version=1.0
Name=Ulti Jarvis
GenericName=Voice Assistant
Comment=Voice assistant that listens for a wake word and runs desktop commands
Exec=/usr/lib/ultijarvis/Ulti-Jarvis-Settings
Icon=ultijarvis
Terminal=false
StartupNotify=true
Categories=Utility;AudioVideo;
Keywords=voice;assistant;speech;jarvis;
")
    install(FILES ${CMAKE_BINARY_DIR}/packaging/ultijarvis.desktop
        DESTINATION share/applications)
    install(FILES ${CMAKE_SOURCE_DIR}/images/ultijarvis.svg
        DESTINATION share/icons/hicolor/scalable/apps)
    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION share/doc/ultijarvis
        RENAME copyright)
    find_program(JARVIS_READELF readelf)
    if(JARVIS_READELF)
        configure_file(${CMAKE_SOURCE_DIR}/cmake/linux_rpaths.cmake.in
            ${CMAKE_BINARY_DIR}/linux_rpaths.cmake @ONLY)
        install(SCRIPT ${CMAKE_BINARY_DIR}/linux_rpaths.cmake)
    else()
        message(WARNING
            "readelf was not found; bundled libraries will keep whatever "
            "absolute search paths the distribution built them with")
    endif()
    file(WRITE ${CMAKE_BINARY_DIR}/packaging/postrm
[=[#!/bin/sh

case "$1" in
    remove|purge|abort-install|abort-upgrade|disappear) ;;
    *) exit 0 ;;
esac

for u in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $2}'); do
    uid=$(id -u "$u" 2>/dev/null) || continue
    runuser -u "$u" -- env XDG_RUNTIME_DIR=/run/user/$uid \
        DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus \
        systemctl --user disable --now ultijarvis-daemon.service \
        >/dev/null 2>&1 || true
done

pkill -f '/usr/lib/ultijarvis/Ulti Jarvis' >/dev/null 2>&1 || true
rm -f /tmp/ultijarvis.pid /tmp/ultijarvis.lock /tmp/ultijarvis.log

if [ "$1" != "purge" ]; then
    exit 0
fi

getent passwd | while IFS=: read -r name pw uid gid gecos home shell; do
    [ "$uid" -ge 1000 ] 2>/dev/null || continue
    [ "$uid" -lt 65534 ] 2>/dev/null || continue
    [ -d "$home" ] || continue
    rm -rf "$home/.config/systemd/user/ultijarvis-daemon.service" \
        "$home/.config/autostart/ultijarvis-daemon.desktop" \
        "$home/.config/ultijarvis" \
        "$home/.cache/Ulti Jarvis" \
        "$home/.cache/Ulti-Jarvis-Settings" \
        "$home/.cache/Ulti-Jarvis-Daemon" \
        "$home/.local/share/ultijarvis" \
        "$home/.local/state/ultijarvis" || true
    loginctl disable-linger "$name" >/dev/null 2>&1 || true
done

exit 0
]=])
    file(CHMOD ${CMAKE_BINARY_DIR}/packaging/postrm
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    list(APPEND CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_BINARY_DIR}/packaging/postrm")
    file(WRITE ${CMAKE_BINARY_DIR}/packaging/rpm_postun
[=[#!/bin/sh

[ "$1" = "0" ] || exit 0

for u in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $2}'); do
    uid=$(id -u "$u" 2>/dev/null) || continue
    runuser -u "$u" -- env XDG_RUNTIME_DIR=/run/user/$uid \
        DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus \
        systemctl --user disable --now ultijarvis-daemon.service \
        >/dev/null 2>&1 || true
done

pkill -f '/usr/lib/ultijarvis/Ulti Jarvis' >/dev/null 2>&1 || true
rm -f /tmp/ultijarvis.pid /tmp/ultijarvis.lock /tmp/ultijarvis.log

getent passwd | while IFS=: read -r name pw uid gid gecos home shell; do
    [ "$uid" -ge 1000 ] 2>/dev/null || continue
    [ "$uid" -lt 65534 ] 2>/dev/null || continue
    [ -d "$home" ] || continue
    rm -rf "$home/.config/systemd/user/ultijarvis-daemon.service" \
        "$home/.config/autostart/ultijarvis-daemon.desktop" \
        "$home/.config/ultijarvis" \
        "$home/.cache/Ulti Jarvis" \
        "$home/.cache/Ulti-Jarvis-Settings" \
        "$home/.cache/Ulti-Jarvis-Daemon" \
        "$home/.local/share/ultijarvis" \
        "$home/.local/state/ultijarvis" || true
    loginctl disable-linger "$name" >/dev/null 2>&1 || true
done

exit 0
]=])
    file(CHMOD ${CMAKE_BINARY_DIR}/packaging/rpm_postun
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
        ${CMAKE_BINARY_DIR}/packaging/rpm_postun)
endif()
configure_file(${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_BINARY_DIR}/LICENSE.txt COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")
if(APPLE)
    set(CPACK_RESOURCE_FILE_README
        "${CMAKE_SOURCE_DIR}/cmake/installer_readme.txt")
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
