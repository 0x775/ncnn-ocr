cmake_minimum_required(VERSION 3.20)
project(OCREngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# compile options
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---- platform tag: x64 / arm64 / armhf ----
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(PLATFORM_TAG "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7|armhf|armv7l)$")
    set(PLATFORM_TAG "armhf")
else()
    set(PLATFORM_TAG "x64")
endif()
message(STATUS "Target platform: ${PLATFORM_TAG} (${CMAKE_SYSTEM_PROCESSOR})")

# output layout: <build>/output/<platform>/{lib,bin}
set(OUTPUT_ROOT "${CMAKE_BINARY_DIR}/output")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_ROOT}/${PLATFORM_TAG}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_ROOT}/${PLATFORM_TAG}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_ROOT}/${PLATFORM_TAG}/bin")

# libs path
if(NOT DEFINED LIB_ROOT)
    if(WIN32)
        set(LIB_ROOT "$ENV{USERPROFILE}/Documents/libs")
    else()
        set(LIB_ROOT "$ENV{HOME}/Documents/libs")
    endif()
endif()

# opencv
if(NOT DEFINED OpenCV_DIR)
    set(OpenCV_DIR "${LIB_ROOT}/opencv/lib/cmake/opencv4")
endif()
find_package(OpenCV REQUIRED)
find_package(Iconv QUIET)
find_package(OpenEXR QUIET)
# openmp
find_package(OpenMP REQUIRED)
# ncnn
if(NOT DEFINED ncnn_DIR)
    set(ncnn_DIR "${LIB_ROOT}/ncnn/lib/cmake/ncnn")
endif()
find_package(ncnn REQUIRED)

# ---- core sources (no main) ----
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/db_net.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/angle_net.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crnn_net.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ocr_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/clipper2/clipper.engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/clipper2/clipper.offset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/clipper2/clipper.rectclip.cpp
)

add_library(ocr_core STATIC ${CORE_SOURCES})
target_include_directories(ocr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty
)
target_link_libraries(ocr_core PUBLIC
    ${OpenCV_LIBS}
    ncnn
    OpenMP::OpenMP_CXX
)

# ---- shared library exporting C API ----
add_library(paddleocr SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/c_api.cpp)
target_include_directories(paddleocr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(paddleocr PRIVATE ocr_core)
target_compile_definitions(paddleocr PRIVATE OCR_BUILD_SHARED)
if(NOT MSVC)
    set_target_properties(paddleocr PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

# ---- original executable ----
add_executable(main ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(main PRIVATE ocr_core)

# ---- demos (host-only, skipped when cross-compiling) ----
option(BUILD_DEMOS "Build demo programs" ON)
if(BUILD_DEMOS AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(examples)
endif()
