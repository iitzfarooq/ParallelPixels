#pragma once // Prevent multiple inclusions

#include<stddef.h> // For size_t
#include<pthread.h> // For pthread types

/**
 * @brief Represents the processing state of an image chunk.
 */
typedef enum {
    CHUNK_STATUS_CREATED,   /**< Chunk has been created by the chunker. */
    CHUNK_STATUS_FILTERED,  /**< Chunk has been processed by the filter stage. */
    CHUNK_STATUS_ERROR,      /**< An error occurred during processing (e.g., filtering). */
    // Add more states if needed, e.g., CHUNK_STATUS_RECONSTRUCTED
} chunk_processing_status_t;

/**
 * @brief Represents a rectangular chunk of a larger image.
 *
 * Contains metadata about the chunk's position and dimensions relative
 * to the original image, as well as a pointer to its own pixel data buffer.
 */
typedef struct {
    // Identification
    int chunk_id;               /**< Unique identifier for this chunk within its original image. */
    char* original_image_name;  /**< Dynamically allocated name of the source image file. Caller must manage memory. */

    // Geometry
    size_t offset_x;            /**< X-coordinate of the top-left corner of this chunk in the original image's coordinate system. */
    size_t offset_y;            /**< Y-coordinate of the top-left corner of this chunk in the original image's coordinate system. */
    size_t width;               /**< Width of this chunk in pixels. */
    size_t height;              /**< Height of this chunk in pixels. */

    // Pixel Data
    unsigned char* pixel_data;  /**< Pointer to the buffer containing the chunk's pixel data (allocated separately). */
    size_t data_size_bytes;     /**< Size of the pixel_data buffer in bytes (width * height * channels). */
    int channels;               /**< Number of color channels per pixel (e.g., 1 for Grayscale, 3 for RGB, 4 for RGBA). */

    // Optional Metadata (Currently commented out)
    int original_image_num_chunks;
    int original_image_width;
    int original_image_height;
    int processing_status;

} image_chunk_t;

/**
 * @brief Node structure for a linked list queue holding image chunks.
 * @note This structure embeds the image_chunk_t directly. For efficiency and
 *       correctness (avoiding unintended copies of potentially large data),
 *       consider storing a pointer (image_chunk_t*) instead.
 * IMPORTANT: The image_chunk_t* is created only once and changes ownership, make sure to free it after reconstruction.
 */
typedef struct chunk_queue_node { // Renamed for clarity
    struct chunk_queue_node* next; /**< Pointer to the next node in the queue, or NULL if this is the last node. */
    image_chunk_t* chunk;           /**< The image chunk data stored in this node. Consider using a pointer instead. */
} chunk_queue_node_t; // Renamed type

/**
 * @brief Manages a thread-safe queue of image chunks.
 *
 * Contains pointers to the head and tail nodes, along with synchronization
 * primitives (mutex and condition variable) for thread-safe operations.
 * IMPORTANT: The image_chunk_t* is created only once and changes ownership, make sure to free it after reconstruction.
 */
typedef struct {
    chunk_queue_node_t *head;       /**< Pointer to the first node in the queue (or NULL if empty). */
    chunk_queue_node_t *tail;       /**< Pointer to the last node in the queue (or NULL if empty). */
    pthread_mutex_t lock;           /**< Mutex to protect access to the queue structure. */
    pthread_cond_t cond_not_empty;  /**< Condition variable to signal when the queue is not empty. */
    // Add cond_not_full if implementing a bounded queue
} chunk_queue_t;

/**
 * @brief Initializes a chunk queue.
 *
 * Sets head and tail to NULL and initializes the mutex and condition variable.
 * Must be called before using the queue.
 *
 * @param q Pointer to the chunk_queue_t structure to initialize.
 * @return 0 on success, non-zero on error initializing mutex or condition variable.
 */
int chunk_queue_init(chunk_queue_t* q);

/**
 * @brief Enqueues an image chunk into the thread-safe queue.
 *
 * Creates a new node, takes the ownership of chunk data pointer,
 * adds it to the tail of the queue, and signals any waiting consumers.
 * IMPORTANT: The image_chunk_t* is created only once and changes ownership, make sure to free it after reconstruction.
 *
 * @param q Pointer to the chunk_queue_t structure.
 * @param c Pointer to the image chunk data to enqueue. The queue takes ownership.
 * @return 0 on success, non-zero on failure (e.g., memory allocation).
 */
int chunk_enqueue(chunk_queue_t* q, image_chunk_t* c);

/**
 * @brief Dequeues an image chunk from the thread-safe queue.
 *
 * Blocks if the queue is empty until an item is available. Removes the head node
 * and returns the contained chunk data.
 *
 * @param q Pointer to the chunk_queue_t structure.
 * @return Pointer to the dequeued image chunk data. The caller is responsible for
 *         managing the memory of the returned chunk and its contents (pixel_data, original_image_name).\
 *         Returns NULL if an error occurs (though typically it blocks).
 */
image_chunk_t* chunk_dequeue(chunk_queue_t* q);

/**
 * @brief Destroys a chunk queue, freeing all nodes and associated resources.
 *
 * Frees all remaining nodes in the queue. Also destroys the mutex and
 * condition variable. This function is NOT thread-safe and should only be
 * called when no other threads are accessing the queue.
 *
 * @param q Pointer to the chunk_queue_t structure to destroy.
 */
void chunk_queue_destroy(chunk_queue_t* q);

// --- Global Shared Variables ---
// These represent shared queues between pipeline stages.
// Ensure they are properly initialized using chunk_queue_init()
// and destroyed using chunk_queue_destroy().

/** @brief Shared queue for passing chunks from the chunker stage to the filtering stage. */
extern chunk_queue_t chunker_filtering_queue;
/** @brief Shared queue for passing chunks from the filtering stage to the reconstruction stage. */
extern chunk_queue_t filtering_reconstruction_queue;