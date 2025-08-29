cmake_minimum_required(VERSION 3.29)

# --- macOS toolchain sanity (no YAML changes needed) ---
# If we're on Apple Silicon and the arch isn't set, default to arm64.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
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

# ===== Windows: force-enable std::byte on MSVC (fixes CI with /D_HAS_STD_BYTE=0) =====
if (MSVC)
    # Undefine any stray value and force it ON (MSVC takes last definition)
    target_compile_options(BloonsFarm PRIVATE /U_HAS_STD_BYTE /D_HAS_STD_BYTE=1)
endif()
# =====================================================================================

# ---------- Windows: EXE icon via .rc ----------
if (WIN32)
  # Absolute path to your .ico in the source tree
  set(APP_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/UI/btdfarmicon.ico")
  # Normalize to forward slashes (rc.exe accepts this)
  file(TO_CMAKE_PATH "${APP_ICON_FILE}" APP_ICON_FILE)

  # Generate an .rc in the *build* dir that contains the absolute path
  set(GENERATED_RC "${CMAKE_CURRENT_BINARY_DIR}/appicon.rc")
  configure_file(
    "${CMAKE_SOURCE_DIR}/resources/appicon.rc.in"
    "${GENERATED_RC}"
    @ONLY
  )

  # Add the *generated* .rc to the target
  target_sources(BloonsFarm PRIVATE "${GENERATED_RC}")
endif()

# ---------- macOS: prefer system bundle icon (optional; currently disabled) ----------
#if (APPLE)
#  set(APP_ICON_MACOS ${CMAKE_SOURCE_DIR}/resources/app.icns)
#  set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
#  target_sources(BloonsFarm PRIVATE ${APP_ICON_MACOS})
#  set_target_properties(BloonsFarm PROPERTIES
#    MACOSX_BUNDLE TRUE
#    MACOSX_BUNDLE_ICON_FILE app.icns
#  )
#endif()

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
        X11 Xtst xcb xkbcommon xkbcommon-x11 Xinerama Xt X11-xcb Xext Xrandr
    )
endif()

# --------------------------------------------------
# ❼ OpenCV build toggles (vendored OpenCV built here)
# --------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs" FORCE)
set(BUILD_opencv_world ON  CACHE BOOL "Build single static lib" FORCE)
set(BUILD_TESTS OFF       CACHE BOOL "Disable tests" FORCE)
set(BUILD_PERF_TESTS OFF  CACHE BOOL "Disable performance tests" FORCE)
set(BUILD_EXAMPLES OFF    CACHE BOOL "Disable examples" FORCE)
set(BUILD_opencv_python OFF CACHE BOOL "Disable python shit" FORCE)
set(BUILD_opencv_apps OFF CACHE BOOL "Disable OpenCV apps" FORCE)

set(WITH_TBB   ON  CACHE BOOL "Enable TBB parallelism" FORCE)
set(WITH_OPENCL ON CACHE BOOL "Enable OpenCL" FORCE)
set(WITH_EIGEN ON CACHE BOOL "Use Eigen for matrix ops" FORCE)

# --- SIMD / CPU features (OpenCV) ---
if(APPLE)
  set(ENABLE_SSE OFF CACHE BOOL "" FORCE)
  set(ENABLE_AVX OFF CACHE BOOL "" FORCE)
  set(CPU_BASELINE "" CACHE STRING "" FORCE)
  set(CPU_DISPATCH "" CACHE STRING "" FORCE)
  set(ENABLE_NEON ON CACHE BOOL "" FORCE)
else()
  set(ENABLE_SSE ON  CACHE BOOL "" FORCE)
  set(ENABLE_AVX ON  CACHE BOOL "" FORCE)
endif()

set(ENABLE_FAST_MATH ON CACHE BOOL "Use fast math" FORCE)

# --- OpenMP (disable on macOS to stay on Apple toolchain) ---
if(APPLE)
  set(WITH_OPENMP OFF CACHE BOOL "Disable OpenMP on macOS" FORCE)
  find_package(OpenMP)  # optional
  if (OpenMP_CXX_FOUND)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
  endif()
else()
  set(WITH_OPENMP ON  CACHE BOOL "Enable OpenMP" FORCE)
  find_package(OpenMP REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

# Disable IPP (causes errors)
set(WITH_IPP   OFF CACHE BOOL "Disable IPP" FORCE)
set(WITH_ITT   OFF CACHE BOOL "Disable ITT" FORCE)
set(WITH_IPP_A OFF CACHE BOOL "Disable IPP Async" FORCE)

add_subdirectory(vendor/opencv ${CMAKE_BINARY_DIR}/vendor/opencv-build)
include_directories(vendor/opencv/include)
set(OpenCV_STATIC ON)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE opencv_world)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_BINARY_DIR}/vendor/opencv
        vendor/opencv/modules/calib3d/include
        vendor/opencv/modules/core/include
        vendor/opencv/modules/dnn/include
        vendor/opencv/modules/features2d/include
        vendor/opencv/modules/flann/include
        vendor/opencv/modules/gapi/include
        vendor/opencv/modules/highgui/include
        vendor/opencv/modules/imgcodecs/include
        vendor/opencv/modules/imgproc/include
        vendor/opencv/modules/java/include
        vendor/opencv/modules/js/include
        vendor/opencv/modules/ml/include
        vendor/opencv/modules/objc/include
        vendor/opencv/modules/objdetect/include
        vendor/opencv/modules/photo/include
        vendor/opencv/modules/python/include
        vendor/opencv/modules/stitching/include
        vendor/opencv/modules/ts/include
        vendor/opencv/modules/video/include
        vendor/opencv/modules/videoio/include
        vendor/opencv/modules/world/include
)

include_directories(${CMAKE_BINARY_DIR}/opencv2)

# -------- macOS: Option A — explicitly link Homebrew libc++/libc++abi --------
if(APPLE)
  # Resolve the compiler to a real path, then check if it's Homebrew LLVM
  get_filename_component(_CXX_REAL "${CMAKE_CXX_COMPILER}" REALPATH)
  if(_CXX_REAL MATCHES "/opt/homebrew/opt/llvm")
    message(STATUS "Detected Homebrew LLVM at: ${_CXX_REAL}")

    # Find the actual dylibs in Homebrew’s prefix (avoid system libc++)
    find_library(HB_LIBCXX    c++    HINTS /opt/homebrew/opt/llvm/lib NO_DEFAULT_PATH REQUIRED)
    find_library(HB_LIBCXXABI c++abi HINTS /opt/homebrew/opt/llvm/lib NO_DEFAULT_PATH REQUIRED)

    # Link explicitly and embed rpath so the loader finds them
    target_link_libraries(BloonsFarm PRIVATE "${HB_LIBCXX}" "${HB_LIBCXXABI}")
    target_link_options(BloonsFarm PRIVATE -Wl,-rpath,/opt/homebrew/opt/llvm/lib)

    # Keep the deployment target consistent with GH mac runners (optional)
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
      set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)
    endif()
  endif()
endif()

# Optional: put binary in /build
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
