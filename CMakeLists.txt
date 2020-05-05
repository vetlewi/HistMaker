cmake_minimum_required(VERSION 3.10)
project(HistMaker)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindPLX.cmake)
#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindXIA.cmake)

if ( APPLE )
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/xia)
else()

    find_library(XIA_APP
        NAMES
            libPixie16App
            Pixie16App
        PATHS
            /opt/xia/current
    )

    find_path(XIA_APP_INCLUDE
        NAMES
            Pixie16App_export.h
        PATHS
            /opt/xia/current/app
    )

    find_library(XIA_SYS
        NAMES
            libPixie16Sys
            Pixie16Sys
        PATHS
            /opt/xia/current
    )

    find_path(XIA_SYS_INCLUDE
            NAMES
            Pixie16Sys_export.h
            PATHS
            /opt/xia/current/sys
    )

    find_library(PLX_LIB
        NAMES
            libPlxApi
            PlxApi
        PATHS
            /opt/plx/current/PlxSdk/PlxApi/Library)

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
            ${XIA_APP} ${XIA_SYS} ${PLX_LIB} spdlog::spdlog indicators::indicators InfluxDB ldl)
endif()
target_compile_features(HistMaker PRIVATE cxx_std_17)
