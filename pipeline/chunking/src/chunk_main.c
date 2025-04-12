#include<image_reader.h>
#include<image_chunker.h>

volatile sig_atomic_t stop_flag = 0;

// --- Safe Signal Handler ---
void ExitHandler(int signum) {
    // Only action: set the flag. Avoid printf, free, etc.
    stop_flag = 1;
    // Optionally, write a simple message using async-signal-safe write()
    const char msg[] = "\nSignal received, initiating shutdown...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(void) {
    const size_t num_threads = 1; // Only creating the watcher thread
    pthread_t watcher;
    // pthread_attr_t watcher_attr; // Not strictly needed if joining
    const char *directoryPath = "../images";

    // --- Setup Safe Signal Handling (using sigaction) ---
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = ExitHandler; // Use the safe handler
    sigfillset(&sa.sa_mask);         // Block other signals during handler execution
    sa.sa_flags = 0;                 // No special flags needed here

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to register SIGINT handler");
        return EXIT_FAILURE;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Failed to register SIGTERM handler");
        // Consider unregistering SIGINT if needed
        return EXIT_FAILURE;
    }

    // --- Directory Check ---
    DIR *dir_check = opendir(directoryPath);
    if (dir_check == NULL) {
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "Target directory '%s' does not exist or cannot be opened", directoryPath);
        perror(err_msg);
        fprintf(stderr, "Please ensure the '../images' directory exists relative to the build directory.\n");
        return EXIT_FAILURE;
    }
    closedir(dir_check);

    // Initialize mutex if needed by queue operations (assuming it's global or passed)
    pthread_mutex_init(&queue_lock, NULL); // If queue_lock is used

    // --- Create Watcher Thread (Consider making it joinable for cleaner shutdown) ---
    printf("Starting image watcher thread for directory: %s\n", directoryPath);
    // pthread_attr_init(&watcher_attr); // Not needed if joining
    // pthread_attr_setdetachstate(&watcher_attr, PTHREAD_CREATE_DETACHED); // Avoid detaching if you want to join

    // Pass the address of the global queue if needed by readImagesFromDirectory
    if (pthread_create(&watcher, NULL, readImagesFromDirectory, (void *)directoryPath) != 0) { // Removed watcher_attr
        perror("Failed to create watcher thread");
        // No cleanup needed here yet as resources aren't fully active
        return EXIT_FAILURE;
    }

    // --- Wait for signal ---
    printf("Watcher thread started. Waiting for signal (SIGINT/SIGTERM)...\n");
    while (!stop_flag) {
        // Sleep for a short duration to avoid busy-waiting
        sleep(1);
    }

    // --- Signal received, proceed with cleanup ---
    printf("\nShutdown signal received.\n");

    // TODO: Implement graceful thread termination
    // Ideally, the readImagesFromDirectory function should check stop_flag
    // and exit its loop. Then you can join the thread here.
    // For now, we proceed directly, but the watcher thread might still be running.
    printf("Attempting to cancel watcher thread (if possible)...\n");
    // pthread_cancel(watcher); // Option 1: Force cancellation (use with care)
    pthread_join(watcher, NULL); // Option 2: Wait if thread checks stop_flag and exits

    printf("Cleaning up resources...\n");

    // Call cleanup functions safely outside the signal handler
    free_processed_files(); // Free the hash table
    free_queue(&queue); // Free the image queue (pass pointer if needed)

    // Destroy mutex if initialized
    pthread_mutex_destroy(&queue_lock);

    printf("Cleanup complete. Exiting.\n");

    return 0; // Normal exit after cleanup
}