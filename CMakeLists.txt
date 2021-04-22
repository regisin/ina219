cmake_minimum_required(VERSION 3.10)

project(ina219
    VERSION 1.0
    LANGUAGES CXX
)

add_library(ina219 SHARED
    "${PROJECT_SOURCE_DIR}/src/ina219.cc"
)

target_include_directories(ina219
    PUBLIC
    "${PROJECT_SOURCE_DIR}/src/"
)

add_executable(profiler
    "${PROJECT_SOURCE_DIR}/profiler.cc"
)

target_link_libraries(profiler
    ina219
)
