cmake_minimum_required(VERSION 3.16)

project(FloodOps LANGUAGES CXX)

option(FLOODOPS_USE_SFML "Build the SFML 3 graphical dashboard when available" ON)

set(FLOODOPS_SOURCES
    LinkedList.cpp
    LocationManagement.cpp
    PriorityQueue.cpp
    Queue.cpp
    RequestHashTable.cpp
    RequestManagement.cpp
    ShelterCapacityTracker.cpp
    Stack.cpp
    main.cpp
)

add_executable(floodops ${FLOODOPS_SOURCES})
target_compile_features(floodops PRIVATE cxx_std_11)

if(FLOODOPS_USE_SFML)
    find_package(SFML 3 COMPONENTS Graphics Window System QUIET)
endif()

if(FLOODOPS_USE_SFML AND SFML_FOUND)
    target_compile_features(floodops PRIVATE cxx_std_17)
    target_compile_definitions(floodops PRIVATE FLOODOPS_ENABLE_SFML=1)
    target_link_libraries(floodops PRIVATE SFML::Graphics SFML::Window SFML::System)
    message(STATUS "Building FloodOps with the SFML 3 dashboard")
else()
    message(STATUS "SFML 3 not found or disabled; building the console interface")
endif()
