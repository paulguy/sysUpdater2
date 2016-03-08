cmake_minimum_required(VERSION 2.8)
project(sysUpdater2)
enable_language(ASM)

set(APP_TITLE "sysUpdater2")
set(APP_AUTHOR "paulguy")
set(APP_DESCRIPTION "Firmware installer based on SafeSysUpdater.")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(APP_ICON res/icon.png)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DKA_SUGGESTED_C_FLAGS}") # Use the devkitArm suggested flags. This is not mandatory.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")

# Note that you must copy the cmake folder and the DevkitArm3DS.cmake file in this directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake) # Add the cmake folder to the modules paths, so that we can use the tools and find_package for ctrulib
include(Tools3DS) # Include all the macros and tools needed for 3ds development.

find_package(CTRULIB REQUIRED) # Look for ctrulib

file(GLOB_RECURSE SOURCE_FILES
        source/*
)
add_executable(sysUpdater2 ${SOURCE_FILES}) # Create the elf file

target_link_libraries(sysUpdater2 ${LIBCTRU_LIBRARIES}) # Link ctrulib

target_include_directories(sysUpdater2 PUBLIC ${LIBCTRU_INCLUDE_DIRS}) # Add the ctrulib include folder

add_3dsx_target(sysUpdater2) # Generate the .3dsx and .smdh from the CtrManager target
add_cia_target(sysUpdater2 res/app.rsf res/banner.png res/audio.wav)
set_target_properties(sysUpdater2 PROPERTIES COMPILE_FLAGS "-DCITRA")
