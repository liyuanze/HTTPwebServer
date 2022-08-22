cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("Setting default build type to Release")
endif()
aux_source_directory(. MAIN) 
project(webServer VERSION 0.0.1 LANGUAGES C CXX)

add_subdirectory(./connection)
add_subdirectory(./threadPool)

add_executable(HTTPserver ${MAIN})

target_link_libraries(HTTPserver libconnection)
target_link_libraries(HTTPserver libthreadpool)
