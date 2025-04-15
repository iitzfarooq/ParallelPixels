#include "filter.h"

#include <stdio.h>

extern chunk_queue_t filtering_reconstruction_queue;

void greyscale(image_chunk_t* chunk) {
    printf("in filter");
    if (!chunk) {
        fprintf(stderr, "Error: chunk is NULL\n");
        return;
    }

    if (!chunk->pixel_data) {
        fprintf(stderr, "Error: pixel_data is NULL\n");
        return;
    }

    int width = chunk->width; 
    int height = chunk->height;
    int offset_x = chunk->offset_x;
    int offset_y = chunk->offset_y;

    for (int i=offset_y; i<=height-1; i++) {
        for (int j=offset_x; j<=width-1; j+=3) {
            unsigned char* pixel = chunk->pixel_data + (i * width + j) * 3; // Assuming RGB format
            unsigned char gray = (unsigned char)(0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2]);
            pixel[0] = gray;
            pixel[1] = gray;
            pixel[2] = gray;
        }
    }

    if (chunk_enqueue(&filtering_reconstruction_queue, chunk)) {
        fprintf(stderr, "Error: Failed to enqueue filtered chunk (ID: %d).\n", chunk->chunk_id);
        free_image_chunk(chunk); // Free the chunk if enqueueing fails
    };
}