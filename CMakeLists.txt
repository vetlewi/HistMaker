cmake_minimum_required(VERSION 3.10)
project(HistMaker)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindPLX.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindXIA.cmake)

if ( APPLE )
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/xia)
else()
    find_package(PLX REQUIRED)
    find_package(XIA REQUIRED)
endif( APPLE )
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/indicators)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/influxdb-cxx)


add_executable(HistMaker
        src/main.cpp
        src/ConfigReader.cpp
        src/xia_interface.cpp
        src/ScalerTransmitter.cpp)

target_include_directories(HistMaker
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        /opt/xia/current/inc
        /opt/xia/current/app
        /opt/xia/current/sys)


target_link_libraries(HistMaker PRIVATE spdlog::spdlog indicators::indicators InfluxDB)
if ( APPLE )
    target_link_libraries(HistMaker PRIVATE
            OCLDAQ::plx OCLDAQ::xia spdlog::spdlog indicators::indicators InfluxDB)
else()
    target_link_libraries(HistMaker PRIVATE
            plx:libplx xia::libpixie spdlog::spdlog indicators::indicators InfluxDB)
endif()
target_compile_features(HistMaker PRIVATE cxx_std_14)
#set_target_properties(HistMaker PROPERTIES CXX_STANDARD 14)