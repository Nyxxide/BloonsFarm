cmake_minimum_required(VERSION 3.29)
project(BloonsFarm++)

set(CMAKE_CXX_STANDARD 20)

# --------------------------------------------------
# ❶ Find OpenCV
# --------------------------------------------------
find_package(OpenCV REQUIRED)

# --------------------------------------------------
# ❷ Include directories
# --------------------------------------------------
include_directories(
        include
        libs/jsonParser
        ${CMAKE_SOURCE_DIR}/libs/libuiohook/include
        ${CMAKE_SOURCE_DIR}/libs/screen_capture_lite/include
        ${OpenCV_INCLUDE_DIRS}
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
)

include_directories(include)

# --------------------------------------------------
# ❹ Qt Setup
# --------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_add_executable(BloonsFarm++ ${SOURCES})
target_link_libraries(BloonsFarm++ PRIVATE Qt6::Widgets)

# --------------------------------------------------
# ❺ libuiohook (per platform)
# --------------------------------------------------
if (WIN32)
    target_link_libraries(BloonsFarm++ PRIVATE
            ${CMAKE_SOURCE_DIR}/libs/libuiohook/lib/uiohook.lib
    )
elseif (APPLE)
    target_link_libraries(BloonsFarm++ PRIVATE
            ${CMAKE_SOURCE_DIR}/libs/libuiohook/lib/libuiohook.a
            "-framework CoreFoundation"
            "-framework CoreGraphics"
    )
elseif (UNIX)
    target_link_libraries(BloonsFarm++ PRIVATE
            ${CMAKE_SOURCE_DIR}/libs/libuiohook/lib/libuiohook.a
            X11 Xtst xcb xkbcommon xkbcommon-x11 Xinerama Xt X11-xcb
    )
endif()

# --------------------------------------------------
# ❻ screen_capture_lite
# --------------------------------------------------
if (WIN32)
    target_link_libraries(BloonsFarm++ PRIVATE
            ${CMAKE_SOURCE_DIR}/libs/screen_capture_lite/lib/screen_capture_lite.lib
            user32 gdi32
    )
elseif (APPLE)
    target_link_libraries(BloonsFarm++ PRIVATE
            ${CMAKE_SOURCE_DIR}/libs/screen_capture_lite/lib/libscreen_capture_lite.a
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework CoreVideo"
            "-framework CoreMedia"
    )
elseif (UNIX)
    target_link_libraries(BloonsFarm++ PRIVATE
            ${CMAKE_SOURCE_DIR}/libs/screen_capture_lite/lib/libscreen_capture_lite.a
            X11 Xfixes pthread
    )
endif()

# --------------------------------------------------
# ❼ Link OpenCV last
# --------------------------------------------------
target_link_libraries(BloonsFarm++ PRIVATE ${OpenCV_LIBS})

# Optional: put binary in /build
set_target_properties(BloonsFarm++ PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
