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
# ------------------------------------------------------------
option(SUPERBUILD_BUILD_QT      "Build Qt in this run"      ON)
option(SUPERBUILD_BUILD_OPENCV  "Build OpenCV in this run"  ON)

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
    list(APPEND QT_COMMON_ARGS -DQT_FEATURE_xcb=ON)
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
# Build & install qtbase (guarded)
# ------------------------------------------------------------
set(APP_DEPS "")  # will append to this list when we actually build deps

if(SUPERBUILD_BUILD_QT)
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
# OpenCV (static, world) — build & install (guarded)
# ------------------------------------------------------------
set(OPENCV_COMMON_ARGS
    -DBUILD_SHARED_LIBS=OFF
    -DCMAKE_INSTALL_PREFIX=${OPENCV_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_opencv_world=ON
    -DBUILD_TESTS=OFF
    -DBUILD_PERF_TESTS=OFF
    -DBUILD_EXAMPLES=OFF
    -DBUILD_opencv_python=OFF
    -DBUILD_opencv_apps=OFF
    -DWITH_IPP=OFF
    -DWITH_ITT=OFF
    -DWITH_IPP_A=OFF
    -DWITH_TBB=ON
    -DWITH_OPENCL=ON
    -DWITH_EIGEN=ON
    -DENABLE_FAST_MATH=ON
    -Wno-dev
)

if(APPLE)
    list(APPEND OPENCV_COMMON_ARGS
        -DCMAKE_C_COMPILER=/usr/bin/clang
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++
        -DCMAKE_OSX_ARCHITECTURES=arm64
        -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
        -DWITH_OPENMP=OFF
        -DENABLE_SSE=OFF
        -DENABLE_AVX=OFF
        -DCPU_BASELINE=
        -DCPU_DISPATCH=
        -DENABLE_NEON=ON
    )
else()
    list(APPEND OPENCV_COMMON_ARGS
        -DWITH_OPENMP=ON
    )
endif()

if (WIN32)
    list(APPEND OPENCV_COMMON_ARGS
        -DCMAKE_MSVC_RUNTIME_LIBRARY=${_WIN_MSVC_RUNTIME}
    )
endif()

if(SUPERBUILD_BUILD_OPENCV)
    ExternalProject_Add(opencv_static
        SOURCE_DIR   ${CMAKE_SOURCE_DIR}/../vendor/opencv
        BINARY_DIR   ${OPENCV_BUILD_DIR}
        CMAKE_ARGS   ${QT_GEN_ARGS} ${OPENCV_COMMON_ARGS}
        BUILD_COMMAND     ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
        INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
        USES_TERMINAL_INSTALL TRUE
    )
    list(APPEND APP_DEPS opencv_static)
endif()

# ------------------------------------------------------------
# Configure & build your app (depends on whatever we build)
# ------------------------------------------------------------
# Always pass both prefixes so the app can consume cached installs
set(_APP_ARGS
    ${QT_GEN_ARGS}
    -DCMAKE_PREFIX_PATH=${QT_INSTALL_PREFIX};${OPENCV_INSTALL_PREFIX}
    -DQt6_DIR=${QT_INSTALL_PREFIX}/lib/cmake/Qt6
    -DOpenCV_DIR=${OPENCV_INSTALL_PREFIX}/lib/cmake/opencv4
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

if(APPLE)
    list(APPEND _APP_ARGS
        -DCMAKE_C_COMPILER=/usr/bin/clang
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++
        -DCMAKE_OSX_ARCHITECTURES=arm64
        -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
    )
endif()

if (WIN32)
    list(APPEND _APP_ARGS
        ${APP_MSVC_RUNTIME_ARG}
        -DQT_STATIC=ON
        -DCMAKE_CXX_FLAGS=/U_HAS_STD_BYTE\ /D_HAS_STD_BYTE=1
    )
endif()

# Only add DEPENDS if the list is non-empty
set(_APP_DEPENDS_ARGS "")
if (APP_DEPS)
    set(_APP_DEPENDS_ARGS DEPENDS ${APP_DEPS})
endif()

ExternalProject_Add(app
    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/..          # your repo root (app CMakeLists.txt)
    BINARY_DIR  ${APP_BUILD_DIR}
    CMAKE_ARGS  ${_APP_ARGS}
    BUILD_COMMAND   ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
    INSTALL_COMMAND ""
    ${_APP_DEPENDS_ARGS}
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE
)

add_custom_target(build-all DEPENDS app)
