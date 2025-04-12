#include <stddef.h> // For size_t

typedef struct {
    // Identification
    int chunk_id;
    char* original_image_name; // Remember to manage memory for this string

    // Geometry
    size_t offset_x; // Top-left corner X in original image
    size_t offset_y; // Top-left corner Y in original image
    size_t width;    // Width of this chunk
    size_t height;   // Height of this chunk

    // Pixel Data
    unsigned char* pixel_data; // Pointer to chunk's pixel buffer
    size_t data_size_bytes;    // Size of pixel_data buffer
    int channels;              // e.g., 1 (Gray), 3 (RGB), 4 (RGBA)

    // Optional Metadata
    // int original_image_width;
    // int original_image_height;
    // int processing_status;

} image_chunk_t;