cmake_minimum_required(VERSION 3.29)

# --- macOS toolchain: force AppleClang + arm64 ---
if(APPLE)
  set(CMAKE_C_COMPILER  "/usr/bin/clang"   CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "" FORCE)
  set(CMAKE_OSX_ARCHITECTURES "arm64"       CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0"    CACHE STRING "" FORCE)
endif()

project(BloonsFarm)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    #Does not support -march=native
else()
    add_compile_options(-march=native)
endif()

# --------------------------------------------------
# ❷ Include directories
# --------------------------------------------------
include_directories(
    include
    include/jsonParser
)

# --------------------------------------------------
# ❸ Source files
# --------------------------------------------------
set(SOURCES
    src/main.cpp
    src/CoordinateHandler.cpp
    src/BloonsUIPopup.cpp
    src/BloonsUICoords.cpp
    src/BloonsUIMain.cpp
    src/ScreenGrabber.cpp
)

include_directories(include)

# --------------------------------------------------
# ❹ Qt Setup
# --------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)
if(WIN32)
    qt_add_executable(BloonsFarm WIN32 ${SOURCES})
else()
    qt_add_executable(BloonsFarm ${SOURCES})
endif()

# ===== Windows: force-enable std::byte on MSVC =====
if (MSVC)
    target_compile_options(BloonsFarm PRIVATE /U_HAS_STD_BYTE /D_HAS_STD_BYTE=1)
endif()

# ---------- Windows: EXE icon via .rc ----------
if (WIN32)
  set(APP_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/UI/btdfarmicon.ico")
  file(TO_CMAKE_PATH "${APP_ICON_FILE}" APP_ICON_FILE)
  set(GENERATED_RC "${CMAKE_CURRENT_BINARY_DIR}/appicon.rc")
  configure_file(
    "${CMAKE_SOURCE_DIR}/resources/appicon.rc.in"
    "${GENERATED_RC}"
    @ONLY
  )
  target_sources(BloonsFarm PRIVATE "${GENERATED_RC}")
endif()

# ---------- macOS bundle icon (optional, disabled) ----------
# (unchanged)

target_link_libraries(BloonsFarm PRIVATE Qt6::Widgets)

# ---- resource auto-generation ----
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(GENERATED_QRC "${CMAKE_BINARY_DIR}/resources.qrc")

file(GLOB_RECURSE RESOURCE_DEPS CONFIGURE_DEPENDS
    "${RESOURCE_DIR}/*.*")

add_custom_command(
    OUTPUT  ${GENERATED_QRC}
    COMMAND ${CMAKE_COMMAND}  -DRESOURCE_DIR=${RESOURCE_DIR}
                              -DOUTPUT_QRC=${GENERATED_QRC}
                              -P ${CMAKE_SOURCE_DIR}/cmake/generate_qrc.cmake
    DEPENDS ${RESOURCE_DEPS}
    COMMENT "Generating resources.qrc"
    VERBATIM)

set_source_files_properties(${GENERATED_QRC} PROPERTIES GENERATED TRUE)
qt_add_resources(EMBEDDED_RES "${GENERATED_QRC}")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${EMBEDDED_RES})

# --------------------------------------------------
# ❺ libuiohook connection
# --------------------------------------------------
add_subdirectory(vendor/libuiohook)
include_directories(vendor/libuiohook/include)

if (WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE uiohook)
elseif (APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        uiohook
        "-framework CoreFoundation"
        "-framework CoreGraphics"
    )
elseif (UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        uiohook
        X11 Xtst xcb xkbcommon xkbcommon-x1 1 Xinerama Xt X11-xcb Xext Xrandr
    )
endif()

# --------------------------------------------------
# ❻ OpenCV: now consume via find_package (static from superbuild)
# --------------------------------------------------
# Superbuild passes:
#   -DCMAKE_PREFIX_PATH=<qt_install>;<opencv_install>
#   -DOpenCV_DIR=<opencv_install>/lib/cmake/opencv4
# So this resolves to the static config we installed.
find_package(OpenCV CONFIG REQUIRED)

# Prefer the world lib we built; fall back gracefully if needed.
if (TARGET opencv_world)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE opencv_world)
elseif (TARGET OpenCV::opencv_world)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OpenCV::opencv_world)
elseif (TARGET opencv_core)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs)
else()
    message(FATAL_ERROR "OpenCV targets not found. Check OpenCV_DIR / CMAKE_PREFIX_PATH.")
endif()

# Optional: put binary in /build
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
