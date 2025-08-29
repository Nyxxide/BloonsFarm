cmake_minimum_required(VERSION 3.24)
project(QtSuperbuild LANGUAGES C CXX)

include(ExternalProject)

# ============================================================
# Global defaults
# ============================================================
# Make sure single-config generators (Ninja) get a build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Always use Ninja for inner builds (Qt + app)
set(QT_GEN_ARGS -G Ninja)

# Prefer static libs unless a child overrides
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ------------------------------------------------------------
# Locations (created automatically)
# ------------------------------------------------------------
set(QT_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/_qt" CACHE PATH "")
set(QTBASE_BUILD_DIR      "${CMAKE_BINARY_DIR}/_qtbuild-qtbase")
set(QTSVG_BUILD_DIR       "${CMAKE_BINARY_DIR}/_qtbuild-qtsvg")
set(QTSHADER_BUILD_DIR    "${CMAKE_BINARY_DIR}/_qtbuild-qtshadertools")
set(QTDECL_BUILD_DIR      "${CMAKE_BINARY_DIR}/_qtbuild-qtdeclarative")
set(APP_BUILD_DIR         "${CMAKE_BINARY_DIR}/app")

# ------------------------------------------------------------
# What Qt modules to build (ONLY qtbase is required)
# ------------------------------------------------------------
option(BUILD_QTSVG "Build qtsvg" OFF)
option(BUILD_QTSHADERTOOLS "Build qtshadertools (needed for Qt Quick)" OFF)
option(BUILD_QTDECLARATIVE "Build qtdeclarative (Qt Quick/QML)" OFF)

# ------------------------------------------------------------
# Common Qt configure flags (static Qt intent)
# ------------------------------------------------------------
# NOTE: For Qt6 static on CMake, the reliable knobs are:
#   -DBUILD_SHARED_LIBS=OFF            (CMake standard; builds static libs)
#   -DQT_FEATURE_static=ON             (Qt feature gate)
# On MSVC we also add: -DQT_FEATURE_static_runtime=ON  (below, in Windows block)
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

# On Linux, explicitly request the XCB platform plugin (will only build if deps exist)
if(UNIX AND NOT APPLE)
    list(APPEND QT_COMMON_ARGS -DQT_FEATURE_xcb=ON)
endif()

# ============================================================
# WINDOWS-SPECIFIC CONFIGURATION (static CRT, consistent /MT)
# ============================================================
if (WIN32)
    # Force static MSVC runtime for ALL inner builds: /MT (Release) and /MTd (Debug)
    set(_WIN_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Qt static runtime + MSVC runtime selection
    list(APPEND QT_COMMON_ARGS
        -DQT_FEATURE_static_runtime=ON
        -DCMAKE_MSVC_RUNTIME_LIBRARY=${_WIN_MSVC_RUNTIME}
    )

    # Reuse for app configure step
    set(APP_MSVC_RUNTIME_ARG -DCMAKE_MSVC_RUNTIME_LIBRARY=${_WIN_MSVC_RUNTIME})

    # (Arch note) With Ninja, the selected VS "Native Tools" env controls x86/x64.
    # Use an x64 Native Tools prompt if you want 64-bit outputs.
endif()
# ========================= END WINDOWS BLOCK =============================

# ------------------------------------------------------------
# Build & install qtbase ONLY (no qttools -> no libclang)
# ------------------------------------------------------------
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

# ------------------------------------------------------------
# Optional: qtsvg (depends on qtbase)
# IMPORTANT: tell it exactly where the freshly installed Qt is
# ------------------------------------------------------------
if (BUILD_QTSVG)
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
endif()

# ------------------------------------------------------------
# Optional: qtshadertools (needed by qtdeclarative/Quick)
# ------------------------------------------------------------
if (BUILD_QTSHADERTOOLS)
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
endif()

# ------------------------------------------------------------
# Optional: qtdeclarative (Qt Quick/QML) — depends on qtbase (+ qtshadertools)
# ------------------------------------------------------------
if (BUILD_QTDECLARATIVE)
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
endif()

# ------------------------------------------------------------
# Configure & build your existing project at repo root (..)
# ------------------------------------------------------------
set(APP_DEPS qtbase_static)
if (BUILD_QTSVG)
    list(APPEND APP_DEPS qtsvg_static)
endif()
if (BUILD_QTDECLARATIVE)
    list(APPEND APP_DEPS qtdeclarative_static)
endif()

# Extra args for app configure
set(_APP_ARGS
    ${QT_GEN_ARGS}
    -DCMAKE_PREFIX_PATH=${QT_INSTALL_PREFIX}
    -DQt6_DIR=${QT_INSTALL_PREFIX}/lib/cmake/Qt6   # hard-pin to *this* Qt
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)
if (WIN32)
    list(APPEND _APP_ARGS
        ${APP_MSVC_RUNTIME_ARG}
        -DQT_STATIC=ON          # let your app conditionally add -DQT_STATIC if it wants
        -DCMAKE_CXX_FLAGS=/U_HAS_STD_BYTE\ /D_HAS_STD_BYTE=1
    )
endif()

ExternalProject_Add(app
    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/..    # your current top-level CMakeLists.txt
    BINARY_DIR  ${APP_BUILD_DIR}
    CMAKE_ARGS  ${_APP_ARGS}
    BUILD_COMMAND   ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
    INSTALL_COMMAND ""
    DEPENDS ${APP_DEPS}
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE
)

# Convenience target
add_custom_target(build-all DEPENDS app)
