cmake_minimum_required(VERSION 3.12)
project(FileSearch)

IF (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Build type not set - defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build from: Debug Release" FORCE)
ENDIF ()


set(CMAKE_CXX_STANDARD 17)

set(BOOST_MIN_VERSION "1.65")
set(Boost_USE_MULTITHREADED OFF)

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost ${BOOST_MIN_VERSION}
        COMPONENTS
        system
        chrono
        filesystem
        thread
        program_options
        log_setup
        log
        REQUIRED)

find_package(Threads REQUIRED)

add_definitions(-DBOOST_LOG_DYN_LINK)

add_executable(FileSearch main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
