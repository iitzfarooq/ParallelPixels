#include<image_chunker.h>
#include<stdlib.h>
#include<image.h>
#include<stdio.h>
#include<errno.h> 

int chunk_queue_init(chunk_queue_t* q) {
    if (q == NULL) 
        return EINVAL; // Use standard error codes if possible

    q->head = NULL;
    q->tail = NULL;

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        perror("chunk_queue_init: Failed to initialize mutex");
        return -1;
    }

    if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
        perror("chunk_queue_init: Failed to initialize condition variable");
        pthread_mutex_destroy(&q->lock); // Clean up mutex
        return -1;
    }

    return 0;
}

int chunk_enqueue(chunk_queue_t* q, image_chunk_t* c) {
    if (q == NULL || c == NULL) 
        return EINVAL;

    // Allocate node *before* locking to minimize lock duration
    chunk_queue_node_t *new_node = (chunk_queue_node_t*)malloc(sizeof(chunk_queue_node_t));
    if (new_node == NULL) {
        perror("chunk_enqueue: Failed to allocate memory for new queue node");
        // The caller still owns 'c' in this case
        return -1;
    }
    new_node->next = NULL;
    new_node->chunk = c; // Assign the incoming chunk pointer

    // --- Critical Section ---
    pthread_mutex_lock(&q->lock);

    if (q->tail == NULL) { // Queue is empty
        if (q->head != NULL) {
            // This indicates a programming error or corruption
            fprintf(stderr, "chunk_enqueue: Queue inconsistency detected (tail is NULL, head is not)\n");
            pthread_mutex_unlock(&q->lock);
            // Free only the node, the caller still owns 'c' as enqueue failed
            free(new_node);
            return -1; // Indicate inconsistency
        }
        q->head = new_node;
        q->tail = new_node;
    } else { // Queue is not empty
        q->tail->next = new_node;
        q->tail = new_node;
    }

    // Signal that the queue is no longer empty
    pthread_cond_signal(&q->cond_not_empty);

    pthread_mutex_unlock(&q->lock);
    // --- End Critical Section ---

    printf("Chunk enqueued successfully (ID: %d)\n", c->chunk_id); // Debugging

    // Ownership of 'c' has now been transferred to the queue
    return 0;
}

image_chunk_t* chunk_dequeue(chunk_queue_t* q) {
    if (q == NULL) 
        return NULL;

    // --- Critical Section ---
    pthread_mutex_lock(&q->lock);

    // Wait while the queue is empty
    while (q->head == NULL) 
        // printf("dequeue: Chunk queue is empty, waiting...\n"); // Debugging
        pthread_cond_wait(&q->cond_not_empty, &q->lock);
        // Re-check condition after waking up

    // Dequeue the node
    chunk_queue_node_t* dequeue_node = q->head;
    image_chunk_t* chunk = dequeue_node->chunk; // Retrieve the chunk pointer

    q->head = dequeue_node->next;

    // If the queue becomes empty after dequeue
    if (q->head == NULL) 
        q->tail = NULL;

    pthread_mutex_unlock(&q->lock);
    // --- End Critical Section ---

    free(dequeue_node); // Free the node structure itself

    printf("Chunk dequeued successfully (ID: %d)\n", chunk->chunk_id); // Debugging

    // Ownership of 'chunk' is transferred to the caller
    return chunk;
}

void chunk_queue_destroy(chunk_queue_t* q) {
    if (q == NULL) 
        return;

    // No need to lock mutex here, assumes no other threads are active.
    chunk_queue_node_t* curr = q->head;
    chunk_queue_node_t* temp;

    while(curr != NULL) {
        // Free the chunk data using the dedicated function
        // This assumes free_image_chunk handles NULL pointers safely
        free_image_chunk(curr->chunk);
        temp = curr;
        curr = curr->next;
        free(temp);       // Free the node structure
    }

    q->head = NULL;
    q->tail = NULL;

    // Destroy synchronization primitives
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond_not_empty);

    printf("Chunk queue destroyed successfully\n");
}