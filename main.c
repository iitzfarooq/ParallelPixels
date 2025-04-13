#include<signal.h>
#include<stdlib.h>
#include<unistd.h>
#include<dirent.h>
#include<errno.h>
#include<stdio.h>

#include<image.h>
#include<image_queue.h>
#include<file_tracker.h>
#include<image_chunker.h>
#include<directory_monitor.h>

image_name_queue_t name_queue;
chunk_queue_t chunker_filtering_queue;
chunk_queue_t filtering_reconstruction_queue;

volatile sig_atomic_t stop_flag = 0;

void ExitHandler(int signum) {
    stop_flag = 1;
    const char msg[] = "\nSignal received, initiating shutdown...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(void) {
    const size_t num_threads = 1;
    pthread_t watcher, chunker;
    const char *directoryPath = "../images";

    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    const size_t num_chunker_threads = (cores > 1)? (size_t)cores: 2; 

    pthread_t watcher_thread;
    pthread_t *chunker_threads = NULL;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = ExitHandler; 
    sigfillset(&sa.sa_mask);       
    sa.sa_flags = 0;                

    if (sigaction(SIGINT, &sa, NULL) == SIG_ERR) {
        perror("Failed to register SIGINT handler");
        return EXIT_FAILURE;
    }

    if (sigaction(SIGTERM, &sa, NULL) == SIG_ERR) {
        perror("Failed to register SIGTERM handler");
        return EXIT_FAILURE;
    }

    DIR *dir_check = opendir(directoryPath);
    if (dir_check == NULL) {
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "Target directory '%s' does not exist or cannot be opened", directoryPath);
        perror(err_msg);
        fprintf(stderr, "Please ensure the '../images' directory exists relative to the build directory.\n");
        return EXIT_FAILURE;
    }

    closedir(dir_check);

    if (image_name_queue_init(&name_queue) != 0) {
        fprintf(stderr, "Failed to initialize name queue.\n");
        return EXIT_FAILURE;
    }

    if (chunk_queue_init(&chunker_filtering_queue) != 0) {
        fprintf(stderr, "Failed to initialize chunker->filtering queue.\n");
        image_name_queue_destroy(&name_queue); // Cleanup previous init
        return EXIT_FAILURE;
    }

    chunker_threads = malloc(num_chunker_threads * sizeof(pthread_t));
    if (chunker_threads == NULL) {
        perror("Failed to allocate memory for chunker thread IDs");
        image_name_queue_destroy(&name_queue);
        chunk_queue_destroy(&chunker_filtering_queue);
        return EXIT_FAILURE;
    }
    
    printf("Starting image watcher thread for directory: %s\n", directoryPath);
    if (pthread_create(&watcher, NULL, read_images_from_directory, (void *)directoryPath) != 0) { // Removed watcher_attr
        perror("Failed to create watcher thread");
        return EXIT_FAILURE;
    }

    printf("Starting %zu chunker threads...\n", num_chunker_threads);
    for (size_t i = 0; i < num_chunker_threads; i++) {
        if (pthread_create(&chunker_threads[i], NULL, chunk_image_thread, NULL) != 0) {
            perror("Failed to create a chunker thread");

            stop_flag = 1; 

            pthread_mutex_lock(&name_queue.lock); 
            pthread_cond_broadcast(&name_queue.cond_not_empty);
            pthread_mutex_unlock(&name_queue.lock); 

            pthread_mutex_lock(&chunker_filtering_queue.lock); 
            pthread_cond_broadcast(&chunker_filtering_queue.cond_not_empty);
            pthread_mutex_unlock(&chunker_filtering_queue.lock); 

            for (size_t j = 0; j < i; ++j) {
                pthread_join(chunker_threads[j], NULL);

            pthread_join(watcher_thread, NULL);
            free(chunker_threads);
            image_name_queue_destroy(&name_queue);
            chunk_queue_destroy(&chunker_filtering_queue);
            return EXIT_FAILURE;
        }
    }


    printf("Watcher thread started. Waiting for signal (SIGINT/SIGTERM)...\n");
    while (!stop_flag) 
        sleep(1);

    printf("\nShutdown signal received.\n");
    printf("Attempting to cancel watcher & chunker thread (if possible)...\n");
    pthread_join(watcher, NULL); 

    printf("Broadcasting to chunker threads...\n");
    pthread_mutex_lock(&name_queue.lock); 
    pthread_cond_broadcast(&name_queue.cond_not_empty);
    pthread_mutex_unlock(&name_queue.lock); 

    pthread_mutex_lock(&chunker_filtering_queue.lock); 
    pthread_cond_broadcast(&chunker_filtering_queue.cond_not_empty);
    pthread_mutex_unlock(&chunker_filtering_queue.lock); 

    printf("Waiting for chunker threads to finish...\n");
    for (size_t i = 0; i < num_chunker_threads; ++i) {
        pthread_join(chunker_threads[i], NULL);
        printf("Chunker thread %zu finished.\n", i);
    }

    printf("All chunker threads finished.\n");

    printf("Cleaning up resources...\n");

    free_processed_files(); 
    image_name_queue_destroy(&name_queue);
    chunk_queue_destroy(&chunker_filtering_queue);

    printf("Cleanup complete. Exiting.\n");

    return 0; 
}