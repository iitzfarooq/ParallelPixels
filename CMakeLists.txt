cmake_minimum_required(VERSION 3.10)
project(ParallelPixels C)

# Set C standard (optional, but good practice)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Find pthreads (likely needed for fork/process management or internal threading)
find_package(Threads REQUIRED)

# --- Executables ---

# Process 1: Chunking (formerly image_reader)
add_executable(chunking
    pipeline/chunking/src/image_chunker.c
    pipeline/chunking/src/image_reader.c
    pipeline/chunking/src/image_table.c
    pipeline/chunking/src/image_queue.c
    pipeline/chunking/src/chunk_main.c
)
# Specify include directories needed by the chunking executable
target_include_directories(chunking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/chunking/include # Its own headers + uthash
    ${CMAKE_CURRENT_SOURCE_DIR}/shared                   # For shared image.h
)
# Link chunking against pthreads
target_link_libraries(chunking PRIVATE Threads::Threads)


# Process 2: Filtering (Placeholder - add sources when available)
# add_executable(filtering
#     pipeline/filtering/src/filtering_main.c # Example source file
# )
# Specify include directories needed by the filtering executable
# target_include_directories(filtering PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/filtering/include # Its own headers
#     ${CMAKE_CURRENT_SOURCE_DIR}/shared                   # For shared image.h
# )
# Link filtering against pthreads
# target_link_libraries(filtering PRIVATE Threads::Threads)


# Process 3: Reconstruction (Placeholder - add sources when available)
# add_executable(reconstruction
#     pipeline/reconstruction/src/reconstruction_main.c # Example source file
# )
# Specify include directories needed by the reconstruction executable
# target_include_directories(reconstruction PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/reconstruction/include # Its own headers
#     ${CMAKE_CURRENT_SOURCE_DIR}/shared                         # For shared image.h
# )
# Link reconstruction against pthreads
# target_link_libraries(reconstruction PRIVATE Threads::Threads)


# Main Orchestrator Process
add_executable(main # Renamed from 'main' for clarity
    main.c
)
# Specify include directories needed by the main executable
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shared # For shared image.h
)
# Link main orchestrator against pthreads (likely needed for fork/wait)
target_link_libraries(main PRIVATE Threads::Threads)


# --- Optional: Debugging ---
# Add -g flag for debugging builds across all targets
# Use cmake .. -DCMAKE_BUILD_TYPE=Debug for a cleaner way
# set(CMAKE_C_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_DEBUG "-g")