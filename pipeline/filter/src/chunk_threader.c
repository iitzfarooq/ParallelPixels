#include <pthread.h>
#include <image.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

extern volatile sig_atomic_t stop_flag;

void *process_chunk(void *arg) {
    image_chunk_t *chunk = (image_chunk_t *)arg;

    // Process the chunk (e.g., apply greyscale filter)
    greyscale(chunk);

    // Enqueue the filtered chunk into the next queue
    if (chunk_enqueue(&filtering_reconstruction_queue, chunk) != 0) {
        fprintf(stderr, "Error: Failed to enqueue filtered chunk (ID: %d).\n", chunk->chunk_id);
        free_image_chunk(chunk); // Free the chunk if enqueueing fails
    }

    return NULL;
}

void assign_threads_to_chunk(void) {
    while (!stop_flag) {
        // Dequeue a chunk from the queue
        image_chunk_t *chunk = chunk_dequeue(&chunker_filtering_queue);
        if (chunk == NULL) {
            // If the queue is empty, wait or check for stop_flag
            continue;
        }

        // Create a new thread for the chunk
        pthread_t thread;
        int result = pthread_create(&thread, NULL, process_chunk, (void *)chunk);
        if (result) {
            fprintf(stderr, "Error creating thread for chunk: %d\n", result);
            free_image_chunk(chunk); // Free the chunk if thread creation fails
            continue;
        }

        // Detach the thread to avoid needing to join it later
        pthread_detach(thread);
    }
}