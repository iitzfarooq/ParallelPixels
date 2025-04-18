cmake_minimum_required(VERSION 3.10)
project(ParallelPipeline C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

# Enable FetchContent for pulling in CityHash
include(FetchContent)

FetchContent_Declare(
    cityhash
    GIT_REPOSITORY git@github.com:google/cityhash.git
    GIT_TAG master
)

FetchContent_MakeAvailable(cityhash)

# Find pthreads (likely needed for threading within the single process)
find_package(Threads REQUIRED)
find_library(MATH_LIBRARY m)

# Main Executable
add_executable(main
    main.c

    pipeline/chunking/src/directory_monitor.c
    pipeline/chunking/src/file_tracker.c
    pipeline/chunking/src/image_chunker.c
    pipeline/filter/src/chunk_threader.c
    pipeline/filter/src/filter.c
    pipeline/chunking/src/image_queue.c

    shared/Object.c
    shared/darrray.c
    shared/dlist.c
    shared/dict.c
    shared/image.c

    pipeline/reconstruction/image_unchunk.c
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/chunking/include
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/filter/include
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/reconstruction

    # Include CityHash headers
    ${cityhash_SOURCE_DIR}
)

# Link against threads, math, and CityHash
target_link_libraries(main PRIVATE
    Threads::Threads
    ${MATH_LIBRARY}
    cityhash
)

# Since CityHash is in C++, force C++ linker for the C-based target
set_target_properties(main PROPERTIES LINKER_LANGUAGE CXX)

# --- Optional: Debugging ---
# Add -g flag for debugging builds across all targets
# Use cmake .. -DCMAKE_BUILD_TYPE=Debug for a cleaner way
# set(CMAKE_C_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_DEBUG "-g")