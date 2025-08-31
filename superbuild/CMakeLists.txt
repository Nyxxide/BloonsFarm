cmake_minimum_required(VERSION 3.24)
project(QtSuperbuild LANGUAGES C CXX)

include(ExternalProject)

# ============================================================
# Global defaults
# ============================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Always use Ninja for inner builds (Qt + OpenCV + app)
set(QT_GEN_ARGS -G Ninja)

# Prefer static libs unless a child overrides
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ------------------------------------------------------------
# Toggle which deps to build in THIS run (YAML controls these)
# Default to OFF so a missing -D flag never triggers an unwanted build
# ------------------------------------------------------------
option(SUPERBUILD_BUILD_QT      "Build Qt in this run"      OFF)
option(SUPERBUILD_BUILD_OPENCV  "Build OpenCV in this run"  OFF)

message(STATUS "SUPERBUILD_BUILD_QT      = ${SUPERBUILD_BUILD_QT}")
message(STATUS "SUPERBUILD_BUILD_OPENCV  = ${SUPERBUILD_BUILD_OPENCV}")

# ------------------------------------------------------------
# Locations (created automatically)
# ------------------------------------------------------------
set(QT_INSTALL_PREFIX       "${CMAKE_BINARY_DIR}/_qt"            CACHE PATH "")
set(OPENCV_INSTALL_PREFIX   "${CMAKE_BINARY_DIR}/_opencv"        CACHE PATH "")

set(QTBASE_BUILD_DIR        "${CMAKE_BINARY_DIR}/_qtbuild-qtbase")
set(QTSVG_BUILD_DIR         "${CMAKE_BINARY_DIR}/_qtbuild-qtsvg")
set(QTSHADER_BUILD_DIR      "${CMAKE_BINARY_DIR}/_qtbuild-qtshadertools")
set(QTDECL_BUILD_DIR        "${CMAKE_BINARY_DIR}/_qtbuild-qtdeclarative")

set(OPENCV_BUILD_DIR        "${CMAKE_BINARY_DIR}/_opencvbuild")
set(APP_BUILD_DIR           "${CMAKE_BINARY_DIR}/app")

# ------------------------------------------------------------
# What Qt modules to build (ONLY qtbase is required)
# ------------------------------------------------------------
option(BUILD_QTSVG "Build qtsvg" OFF)
option(BUILD_QTSHADERTOOLS "Build qtshadertools (needed for Qt Quick)" OFF)
option(BUILD_QTDECLARATIVE "Build qtdeclarative (Qt Quick/QML)" OFF)

# ------------------------------------------------------------
# Common Qt configure flags (static Qt intent)
# ------------------------------------------------------------
set(QT_COMMON_ARGS
        -DBUILD_SHARED_LIBS=OFF
        -DQT_FEATURE_static=ON
        -DCMAKE_INSTALL_PREFIX=${QT_INSTALL_PREFIX}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DQT_BUILD_TESTS=OFF
        -DQT_BUILD_EXAMPLES=OFF
        -DQT_FEATURE_gui=ON
        -DQT_FEATURE_widgets=ON
        -DQT_FEATURE_sql=OFF
        -DQT_FEATURE_opengl=ON
        -Wno-dev
)

if(UNIX AND NOT APPLE)
    # XCB for Linux desktop + turn off ICU to avoid runtime libicu*.so deps
    list(APPEND QT_COMMON_ARGS
            -DQT_FEATURE_xcb=ON
            -DQT_FEATURE_icu=OFF
    )
endif()

# ============================================================
# macOS toolchain / arch / deployment target
# ============================================================
if(APPLE)
    list(APPEND QT_COMMON_ARGS
            -DCMAKE_C_COMPILER=/usr/bin/clang
            -DCMAKE_CXX_COMPILER=/usr/bin/clang++
            -DCMAKE_OSX_ARCHITECTURES=arm64
            -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
    )
endif()

# ============================================================
# WINDOWS-SPECIFIC CONFIGURATION (static CRT, consistent /MT)
# ============================================================
if (WIN32)
    set(_WIN_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    list(APPEND QT_COMMON_ARGS
            -DQT_FEATURE_static_runtime=ON
            -DCMAKE_MSVC_RUNTIME_LIBRARY=${_WIN_MSVC_RUNTIME}
    )
    set(APP_MSVC_RUNTIME_ARG -DCMAKE_MSVC_RUNTIME_LIBRARY=${_WIN_MSVC_RUNTIME})
endif()

# ------------------------------------------------------------
# Build & install qtbase (guarded with source assert)
# ------------------------------------------------------------
set(APP_DEPS "")  # will append to this list when we actually build deps

if(SUPERBUILD_BUILD_QT)
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/../vendor/qt/qtbase/CMakeLists.txt")
        message(FATAL_ERROR
                "SUPERBUILD_BUILD_QT=ON but 'vendor/qt/qtbase' is missing.\n"
                "Make sure the Qt super-repo submodule is initialized recursively "
                "and 'init-repository --module-subset=qtbase' has been run on cache misses.")
    endif()

    ExternalProject_Add(qtbase_static
            SOURCE_DIR   ${CMAKE_SOURCE_DIR}/../vendor/qt/qtbase
            BINARY_DIR   ${QTBASE_BUILD_DIR}
            CMAKE_ARGS   ${QT_GEN_ARGS} ${QT_COMMON_ARGS}
            BUILD_COMMAND     ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            USES_TERMINAL_CONFIGURE TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_INSTALL TRUE
    )
    list(APPEND APP_DEPS qtbase_static)
endif()

# Optional Qt modules only if we're building Qt in this run
if (BUILD_QTSVG AND SUPERBUILD_BUILD_QT)
    ExternalProject_Add(qtsvg_static
            SOURCE_DIR   ${CMAKE_SOURCE_DIR}/../vendor/qt/qtsvg
            BINARY_DIR   ${QTSVG_BUILD_DIR}
            CMAKE_ARGS   ${QT_GEN_ARGS} ${QT_COMMON_ARGS}
            -DQt6_DIR=${QT_INSTALL_PREFIX}/lib/cmake/Qt6
            BUILD_COMMAND     ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            DEPENDS      qtbase_static
            USES_TERMINAL_CONFIGURE TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_INSTALL TRUE
    )
    list(APPEND APP_DEPS qtsvg_static)
endif()

if (BUILD_QTSHADERTOOLS AND SUPERBUILD_BUILD_QT)
    ExternalProject_Add(qtshadertools_static
            SOURCE_DIR   ${CMAKE_SOURCE_DIR}/../vendor/qt/qtshadertools
            BINARY_DIR   ${QTSHADER_BUILD_DIR}
            CMAKE_ARGS   ${QT_GEN_ARGS} ${QT_COMMON_ARGS}
            -DQt6_DIR=${QT_INSTALL_PREFIX}/lib/cmake/Qt6
            BUILD_COMMAND     ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            DEPENDS      qtbase_static
            USES_TERMINAL_CONFIGURE TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_INSTALL TRUE
    )
    list(APPEND APP_DEPS qtshadertools_static)
endif()

if (BUILD_QTDECLARATIVE AND SUPERBUILD_BUILD_QT)
    set(_DECL_DEPS qtbase_static)
    if (BUILD_QTSHADERTOOLS)
        list(APPEND _DECL_DEPS qtshadertools_static)
    endif()
    ExternalProject_Add(qtdeclarative_static
            SOURCE_DIR   ${CMAKE_SOURCE_DIR}/../vendor/qt/qtdeclarative
            BINARY_DIR   ${QTDECL_BUILD_DIR}
            CMAKE_ARGS   ${QT_GEN_ARGS} ${QT_COMMON_ARGS}
            -DQt6_DIR=${QT_INSTALL_PREFIX}/lib/cmake/Qt6
            BUILD_COMMAND     ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            DEPENDS      ${_DECL_DEPS}
            USES_TERMINAL_CONFIGURE TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_INSTALL TRUE
    )
    list(APPEND APP_DEPS qtdeclarative_static)
endif()

# ------------------------------------------------------------
# OpenCV (static, no TBB to avoid libtbb.* at runtime)
# ------------------------------------------------------------
if (SUPERBUILD_BUILD_OPENCV)
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/../vendor/opencv/CMakeLists.txt")
        message(FATAL_ERROR
                "SUPERBUILD_BUILD_OPENCV=ON but 'vendor/opencv' is missing.\n"
                "Initialize the submodule on cache misses.")
    endif()

    # Common OpenCV args (static ‘world’ lib, bundled 3rdparty, no tests/apps/python)
    set(OCV_COMMON_ARGS
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=${OPENCV_INSTALL_PREFIX}
            -DCMAKE_INSTALL_LIBDIR=lib
            -DBUILD_SHARED_LIBS=OFF
            -DBUILD_opencv_world=ON
            -DBUILD_TESTS=OFF
            -DBUILD_PERF_TESTS=OFF
            -DBUILD_EXAMPLES=OFF
            -DBUILD_opencv_python=OFF
            -DBUILD_opencv_apps=OFF
            -DWITH_TBB=OFF           # <<< disable TBB so there is no libtbb runtime dep
            -DWITH_IPP=OFF
            -DWITH_ITT=OFF
            -DWITH_IPP_A=OFF
            -DWITH_EIGEN=ON
            -DWITH_OPENCL=ON
            -DOPENCV_FORCE_3RDPARTY_BUILD=ON
            -DENABLE_FAST_MATH=ON
    )

    if (APPLE)
        list(APPEND OCV_COMMON_ARGS
                -DWITH_OPENMP=OFF                    # AppleClang OpenMP off
                -DCMAKE_OSX_ARCHITECTURES=arm64
                -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
                -DENABLE_SSE=OFF -DENABLE_AVX=OFF
                -DCPU_BASELINE= -DCPU_DISPATCH=
                -DENABLE_NEON=ON
        )
    elseif(UNIX)
        list(APPEND OCV_COMMON_ARGS -DWITH_OPENMP=ON)
    elseif(WIN32)
        list(APPEND OCV_COMMON_ARGS
                -DWITH_OPENMP=ON
                ${APP_MSVC_RUNTIME_ARG}
        )
    endif()

    ExternalProject_Add(opencv_world_static
            SOURCE_DIR   ${CMAKE_SOURCE_DIR}/../vendor/opencv
            BINARY_DIR   ${OPENCV_BUILD_DIR}
            CMAKE_ARGS   ${QT_GEN_ARGS} ${OCV_COMMON_ARGS}
            BUILD_COMMAND     ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
            USES_TERMINAL_CONFIGURE TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_INSTALL TRUE
    )
    list(APPEND APP_DEPS opencv_world_static)
endif()

# ----------------
