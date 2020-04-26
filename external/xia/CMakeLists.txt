cmake_minimum_required(VERSION 3.13)
project(OCLDAQ VERSION 1.0.0 LANGUAGES CXX C)

#Make sure that custom modules are found
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

##############################################
# Declare options

option(ENABLE_MASTER "Build target master" ON)
option(ENABLE_RUPDATE "Build target rupdate" ON)
option(ENABLE_XIAENGINE "Build target XIAengine" ON)
option(ENABLE_XIAONLINE "Build target XIAonline" ON)
option(ENABLE_HISTDUMPER "Build target HistDumper" ON)


option(ENABLE_DOC "Generates the documentation target" OFF)
option(ENABLE_COVERAGE "Generates the coverage build" OFF)
option(ENABLE_TESTING "Turns on testing" OFF)

#set(CMAKE_VERBOSE_MAKEFILE 1)

##############################################
# Declare dependencies


##############################################
# Create target and set properties

add_library(sirius STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sirius/engine_shm.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sirius/net_control.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sirius/run_command.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sirius/sort_spectra.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sirius/utilities.cpp
)

add_library(${PROJECT_NAME}::sirius ALIAS sirius)

target_include_directories(sirius
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sirius
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sirius
)

set_target_properties(sirius PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO)

add_library(plx STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plx/Aardvark.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plx/Eep_8000.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plx/PlxApi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plx/PlxApiI2cAa.c)

add_library(${PROJECT_NAME}::plx ALIAS plx)

target_link_libraries(plx PUBLIC ${CMAKE_DL_LIBS})

target_include_directories(plx
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plx
        ${CMAKE_CURRENT_SOURCE_DIR}/include/plx
)

target_compile_definitions(plx PUBLIC
        -DPLX_LITTLE_ENDIAN
        -DPCI_CODE
        -DPLX_LINUX)

add_library(xia STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xia/communication.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xia/i2cm24c64.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xia/pixie16app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xia/pixie16sys.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xia/tools.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xia/utilities.c
)

add_library(${PROJECT_NAME}::xia ALIAS xia)

target_include_directories(xia
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xia
        ${CMAKE_CURRENT_SOURCE_DIR}/include/xia
)

target_compile_definitions(xia PUBLIC
        -DPLX_LITTLE_ENDIAN
        -DPCI_CODE
        -DPLX_LINUX)

target_link_libraries(xia PUBLIC ${PROJECT_NAME}::plx)