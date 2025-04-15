cmake_minimum_required(VERSION 3.10)
project(ParallelPipeline C) # Changed project name slightly

# Set C standard (optional, but good practice)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

# Find pthreads (likely needed for threading within the single process)
find_package(Threads REQUIRED)
# Find Math library (needed for ceil in chunker)
find_library(MATH_LIBRARY m)

# --- Main Executable ---
add_executable(main
    # Entry Point
    main.c

    # Chunking Module Sources
    pipeline/chunking/src/directory_monitor.c
    pipeline/chunking/src/file_tracker.c
    pipeline/chunking/src/image_chunker.c
    pipeline/filter/src/chunk_threader.c
    pipeline/filter/src/filter.c
    pipeline/chunking/src/image_queue.c # Assuming this is used for inter-thread comms
    #pipeline/filtering/src/filtering.c

    shared/image.c
    # Filtering Module Sources (Add your filtering .c files here)
    # pipeline/filtering/src/filter_logic.c # Example

    # Reconstruction Module Sources (Add your reconstruction .c files here)
    # pipeline/reconstruction/src/reconstructor.c # Example

    # Add any other .c files needed by main
)

# Specify ALL include directories needed by the main executable
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shared                   # Shared types (image.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/chunking/include # Chunking headers + uthash
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/filter/include # Filtering headers
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/reconstruction/include # Reconstruction headers
)

# Link main against pthreads and the math library
target_link_libraries(main PRIVATE
    Threads::Threads
    ${MATH_LIBRARY}
)

# --- Optional: Debugging ---
# Add -g flag for debugging builds across all targets
# Use cmake .. -DCMAKE_BUILD_TYPE=Debug for a cleaner way
# set(CMAKE_C_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_DEBUG "-g")