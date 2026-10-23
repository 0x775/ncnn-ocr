# Demo programs calling the shared library via the C API.
# demo_link / demo_dlopen are plain C; demo_cpp shows C++ RAII usage.

add_executable(demo_link demo_link.c)
target_include_directories(demo_link PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(demo_link PRIVATE paddleocr)

add_executable(demo_dlopen demo_dlopen.c)
target_include_directories(demo_dlopen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(demo_dlopen PRIVATE ${CMAKE_DL_LIBS})

add_executable(demo_cpp demo_cpp.cpp)
target_include_directories(demo_cpp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(demo_cpp PRIVATE paddleocr)
