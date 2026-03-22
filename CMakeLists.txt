cmake_minimum_required(VERSION 4.1)
project(TSP_Project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List of source files
set(SOURCES
        src/config.cpp
        src/graph.cpp
        src/tsplib_parser.cpp
        src/tsp_algorithms.cpp
        src/main.cpp
)

add_executable(tsp_app ${SOURCES})