#pragma once

#include <image.h> // Include the definition of image_chunk_t from shared/image.h
#include <stddef.h> // For size_t
#include <pthread.h> // For pthread_t

// --- Struct to pass input data to the chunker thread ---
typedef struct {
    const char *original_filename;
    unsigned char *image_data;
    int width;
    int height;
    int channels;
    int chunk_width;
    int chunk_height;
} chunker_input_t;

// --- Struct to receive output data from the chunker thread ---
typedef struct {
    image_chunk_t **chunks; // Array of chunk pointers
    int num_chunks_total;   // Total number of chunks created
    int error_flag;         // 0 on success, non-zero on error
} chunker_output_t;


/**
 * @brief Loads an image from a file. (Remains the same)
 *
 * Uses stb_image to load the image. The returned pixel data must be freed
 * using stbi_image_free().
 *
 * @param filename Path to the image file.
 * @param width Pointer to store the image width.
 * @param height Pointer to store the image height.
 * @param channels Pointer to store the number of color channels.
 * @return unsigned char* Pointer to the loaded pixel data, or NULL on failure.
 */
unsigned char *load_image(const char *filename, int *width, int *height, int *channels);

/**
 * @brief Thread function to divide loaded image data into multiple chunks.
 *
 * Takes a pointer to chunker_input_t as input (void* arg).
 * Allocates memory for chunker_output_t and returns a pointer to it (void*).
 * The caller is responsible for freeing the returned chunker_output_t struct
 * AND the chunks array within it (using free_image_chunks).
 *
 * @param arg A void pointer to a chunker_input_t struct.
 * @return void* A void pointer to a newly allocated chunker_output_t struct,
 *               or NULL if the output struct allocation fails.
 */
void *chunk_image_thread(void *arg);


/**
 * @brief Frees the memory allocated for an array of image chunks. (Remains the same)
 *
 * Frees the pixel data, original name string, and struct for each chunk,
 * and then frees the array of pointers itself.
 *
 * @param chunks The array of image_chunk_t pointers.
 * @param num_chunks The total number of chunks in the array.
 */
void free_image_chunks(image_chunk_t **chunks, int num_chunks);
