cmake_minimum_required(VERSION 3.24)
project(QtSuperbuild LANGUAGES C CXX)

include(ExternalProject)

# ------------------------------------------------------------
# Locations (created automatically)
# ------------------------------------------------------------
set(QT_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/_qt" CACHE PATH "")
set(QTBASE_BUILD_DIR      "${CMAKE_BINARY_DIR}/_qtbuild-qtbase")
set(QTSVG_BUILD_DIR       "${CMAKE_BINARY_DIR}/_qtbuild-qtsvg")
set(QTSHADER_BUILD_DIR    "${CMAKE_BINARY_DIR}/_qtbuild-qtshadertools")
set(QTDECL_BUILD_DIR      "${CMAKE_BINARY_DIR}/_qtbuild-qtdeclarative")
set(APP_BUILD_DIR         "${CMAKE_BINARY_DIR}/app")

# Always use Ninja for inner builds (Qt + app)
set(QT_GEN_ARGS -G Ninja)

# ------------------------------------------------------------
# What Qt modules to build (ONLY qtbase is required)
# ------------------------------------------------------------
option(BUILD_QTSVG "Build qtsvg" OFF)
option(BUILD_QTSHADERTOOLS "Build qtshadertools (needed for Qt Quick)" OFF)
option(BUILD_QTDECLARATIVE "Build qtdeclarative (Qt Quick/QML)" OFF)

# ------------------------------------------------------------
# Common Qt configure flags (static Qt)
# ------------------------------------------------------------
set(QT_COMMON_ARGS
        -DQT_BUILD_SHARED_LIBS=OFF
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

# Keep MSVC runtime static only when using MSVC (Windows)
if (MSVC)
    list(APPEND QT_COMMON_ARGS
            -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>
    )
endif()

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
# ------------------------------------------------------------
if (BUILD_QTSVG)
    ExternalProject_Add(qtsvg_static
            SOURCE_DIR   ${CMAKE_SOURCE_DIR}/../vendor/qt/qtsvg
            BINARY_DIR   ${QTSVG_BUILD_DIR}
            CMAKE_ARGS   ${QT_GEN_ARGS} ${QT_COMMON_ARGS}
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

ExternalProject_Add(app
        SOURCE_DIR  ${CMAKE_SOURCE_DIR}/..    # your current top-level CMakeLists.txt
        BINARY_DIR  ${APP_BUILD_DIR}
        CMAKE_ARGS
        ${QT_GEN_ARGS}
        -DCMAKE_PREFIX_PATH=${QT_INSTALL_PREFIX}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        BUILD_COMMAND   ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:>,$<CONFIG>,Release>
        INSTALL_COMMAND ""
        DEPENDS ${APP_DEPS}
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
)

# Convenience target
add_custom_target(build-all DEPENDS app)
