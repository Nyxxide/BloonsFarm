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
src/ScreenGrabber.cpp
)

include_directories(include)

# --------------------------------------------------
# ❹ Qt Setup
# --------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_add_executable(BloonsFarm++ ${SOURCES})
target_link_libraries(BloonsFarm++ PRIVATE Qt6::Widgets)

# ----  resource auto-generation  ----
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(GENERATED_QRC "${CMAKE_BINARY_DIR}/resources.qrc")

# Re-run this rule whenever ANY file in resources/ changes
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

# Mark as "generated" so CLion/Ninja don’t complain
set_source_files_properties(${GENERATED_QRC} PROPERTIES GENERATED TRUE)

# Compile the .qrc into C++ code
qt_add_resources(EMBEDDED_RES "${GENERATED_QRC}")

# Make the exe use that generated .cpp
target_sources(BloonsFarm++ PRIVATE ${EMBEDDED_RES})

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
X11 Xtst xcb xkbcommon xkbcommon-x11 Xinerama Xt X11-xcb Xext Xrandr
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

