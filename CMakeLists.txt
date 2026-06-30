cmake_minimum_required(VERSION 3.21)
project(HagUI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)

add_library(HagUI SHARED
    src/Plugin.cpp
    src/Log.cpp
    src/Hooking.cpp
    src/UI/HagMenu.cpp
    src/api/HagApi.cpp
)

target_include_directories(HagUI PRIVATE src)
target_precompile_headers(HagUI PRIVATE src/PCH.h)
target_compile_definitions(HagUI PRIVATE UNICODE _UNICODE)
target_link_libraries(HagUI PRIVATE spdlog::spdlog minhook::minhook Shell32 Ole32)

# DLL named exactly HagUI.dll (no "lib" prefix)
set_target_properties(HagUI PROPERTIES PREFIX "" OUTPUT_NAME "HagUI")
