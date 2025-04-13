#include<errno.h>
#include<image_queue.h>
#include<chunking_internal.h>

/**
 * @brief Initializes an image name queue.
 */
int image_name_queue_init(image_name_queue_t* q) {
    if (q == NULL) 
        return EINVAL; 

    q->head = NULL;
    q->tail = NULL;

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        perror("image_name_queue_init: Failed to initialize mutex");
        return -1; // Or a specific error code
    }
    if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
        perror("image_name_queue_init: Failed to initialize condition variable");
        pthread_mutex_destroy(&q->lock); // Clean up mutex
        return -1; // Or a specific error code
    }
    
    return 0;
}

/**
 * @brief Enqueues an image filename into the thread-safe queue.
 */
int enqueue_image_name(image_name_queue_t *q, const char *name) {
    if (q == NULL || name == NULL) 
        return EINVAL;

    // Allocate node and name *before* locking to minimize lock duration
    image_name_queue_node_t *new_node = (image_name_queue_node_t*)malloc(sizeof(image_name_queue_node_t));
    if (new_node == NULL) {
        perror("enqueue_image_name: Failed to allocate memory for new queue node");
        return -1;
    }
    new_node->next = NULL;

    // Use strdup for combined allocation and copy (POSIX)
    new_node->name = strdup(name);
    if (new_node->name == NULL) {
        perror("enqueue_image_name: Failed to allocate memory for node name (strdup)");
        free(new_node);
        return -1;
    }

    pthread_mutex_lock(&q->lock);

    if (q->tail == NULL) { 
        if (q->head != NULL) {
            fprintf(stderr, "enqueue_image_name: Queue inconsistency detected (tail is NULL, head is not)\n");
            pthread_mutex_unlock(&q->lock);
            free(new_node->name);
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

    printf("image name enqueued successfully: %s\n", name); // Debugging

    return 0;
}

/**
 * @brief Dequeues an image filename from the thread-safe queue.
 */
char* dequeue_image_name(image_name_queue_t *q) {
    if (q == NULL) 
        return NULL; // Or handle error appropriately

    // --- Critical Section ---
    pthread_mutex_lock(&q->lock);

    // Wait while the queue is empty
    while (q->head == NULL) 
        // printf("dequeue: Queue is empty, waiting...\n"); // Debugging
        pthread_cond_wait(&q->cond_not_empty, &q->lock);
        // Re-check condition after waking up (spurious wakeups or multiple consumers)

    // Dequeue the node
    image_name_queue_node_t* dequeue_node = q->head;
    char* name = dequeue_node->name; // Name ownership is transferred

    q->head = dequeue_node->next;

    // If the queue becomes empty after dequeue
    if (q->head == NULL) 
        q->tail = NULL;

    pthread_mutex_unlock(&q->lock);
    // --- End Critical Section ---

    free(dequeue_node); // Free the node structure itself

    printf("image name dequeued successfully: %s\n", name); // Debugging

    return name; // Caller is responsible for freeing this string
}

/**
 * @brief Destroys an image name queue, freeing all nodes and associated resources.
 */
void image_name_queue_destroy(image_name_queue_t* q) {
    if (q == NULL) 
        return;

    // No need to lock mutex here, this function assumes no other threads are active.
    image_name_queue_node_t* curr = q->head;
    image_name_queue_node_t* temp;

    while(curr != NULL) {
        free(curr->name); // Free the duplicated name string
        temp = curr;
        curr = curr->next;
        free(temp);       // Free the node structure
    }

    q->head = NULL;
    q->tail = NULL;

    // Destroy synchronization primitives
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond_not_empty);

    printf("Image Name queue destroyed successfully\n");

    // Note: The queue structure 'q' itself is not freed here,
    // as it might be allocated statically or on the stack.
    // If 'q' was dynamically allocated, the caller must free it.
}