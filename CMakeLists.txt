cmake_minimum_required(VERSION 3.10)

# Project name and language
project(qdiff C)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")

# Add sources
set(SOURCES
    main.c
    commit_graph_walk.c
)

# Include directories
include_directories(include)

# Create the executable
add_executable(qdiff ${SOURCES})

# Link the required libraries
target_link_libraries(qdiff git2 ncurses)
