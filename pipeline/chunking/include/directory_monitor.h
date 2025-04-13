#pragma once

#include<image_queue.h>
#include<file_tracker.h>
#include<chunking_internal.h>

/**
 * @brief Thread function to continuously scan a directory for new image files
 *        (.jpg, .png) and process them if they haven't been processed before.
 *        Uses functions from image_table.h to track processed files.
 *
 * @param arg A void pointer expected to hold the path (const char *) to the directory to scan.
 * @return void* Always returns NULL in the current implementation (infinite loop).
 */
void *read_images_from_directory(void *arg);