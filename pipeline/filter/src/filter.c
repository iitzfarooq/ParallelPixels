#include "filter.h"

#include <stdio.h>

extern chunk_queue_t filtering_reconstruction_queue;

void greyscale(image_chunk_t* chunk) {
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
    int channels = chunk->channels;

    for (int i=0; i<width*height*channels; i+=channels) {
        unsigned char* pixel = chunk->pixel_data;
        unsigned char gray = (unsigned char)(0.299 * pixel[i+0] + 0.587 * pixel[i+1] + 0.114 * pixel[i+2]); // assuming RGB
        pixel[0] = gray;
        pixel[1] = gray;
        pixel[2] = gray;
    }
}