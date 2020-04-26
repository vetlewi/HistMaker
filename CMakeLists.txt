cmake_minimum_required(VERSION 3.10)
project(HistMaker)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/xia)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/indicators)

add_executable(HistMaker
        src/main.cpp
        src/ConfigReader.cpp
        src/xia_interface.cpp)

target_include_directories(HistMaker
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external)

target_link_libraries(HistMaker PRIVATE OCLDAQ::xia spdlog::spdlog indicators::indicators)
set_target_properties(HistMaker PROPERTIES CXX_STANDARD 11)
