#include<image_reader.h>

void *readImagesFromDirectory(void *arg) {
    const char *directoryPath = (const char *)arg;
    struct dirent *entry;
    DIR *dir = NULL; // Initialize to NULL

    // --- Initial Scan ---
    printf("Thread %lu: Performing initial scan of directory: %s\n", pthread_self(), directoryPath);
    dir = opendir(directoryPath);
    if (dir == NULL) {
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "Thread %lu: Unable to open directory for initial scan (%s)", pthread_self(), directoryPath);
        perror(err_msg);
        return NULL; // Exit thread on error
    }

    while ((entry = readdir(dir)) != NULL) {
        // --- Check stop_flag before processing each entry ---
        if (stop_flag) {
            printf("Thread %lu: Stop flag detected during initial scan.\n", pthread_self());
            break; // Exit the inner loop
        }

        bool should_process = false;
        char current_filename[256];
        strncpy(current_filename, entry->d_name, sizeof(current_filename) - 1);
        current_filename[sizeof(current_filename) - 1] = '\0';

        // Check extension and if not already processed (assuming thread-safe hash table access)
        if (strstr(current_filename, ".jpg") || strstr(current_filename, ".png")) {
            if (!was_file_processed(current_filename)) {
                add_processed_file(current_filename);
                should_process = true;
            }
        }

        if (should_process) {
            char imagePath[1024];
            snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, current_filename);
            // Lock if enqueue needs protection
            pthread_mutex_lock(&queue_lock);
            enqueue(&queue, imagePath);
            pthread_mutex_unlock(&queue_lock);
        }
    }
    
    closedir(dir);
    dir = NULL; // Reset dir pointer

    // --- Check stop_flag before starting monitoring loop ---
    if (stop_flag) {
         printf("Thread %lu: Stop flag detected after initial scan.\n", pthread_self());
         return NULL; // Exit thread function
    }

    printf("Thread %lu: Initial scan complete. Monitoring...\n", pthread_self());

    // --- Monitoring Loop ---
    while (!stop_flag) { // Check stop_flag at the start of each loop iteration
        dir = opendir(directoryPath);
        if (dir == NULL) {
             char err_msg[512];
             snprintf(err_msg, sizeof(err_msg), "Thread %lu: Unable to open directory for monitoring (%s)", pthread_self(), directoryPath);
             perror(err_msg);
             // Check flag before sleeping
             if (stop_flag) break;
             sleep(5); // Still sleep on error
             continue; // Try again after sleep
        }

        while ((entry = readdir(dir)) != NULL) {
            // --- Check stop_flag within the inner loop too ---
             if (stop_flag) {
                 printf("Thread %lu: Stop flag detected during monitoring scan.\n", pthread_self());
                 break; // Exit the inner readdir loop
             }

            bool should_process = false;
            char current_filename[256];
            strncpy(current_filename, entry->d_name, sizeof(current_filename) - 1);
            current_filename[sizeof(current_filename) - 1] = '\0';

            // Check extension and if not already processed (assuming thread-safe hash table access)
            if (strstr(current_filename, ".jpg") || strstr(current_filename, ".png")) {
                 // Lock if was_file_processed/add_processed_file need protection
                 // pthread_mutex_lock(&file_table_lock); // Example lock
                if (!was_file_processed(current_filename)) {
                    add_processed_file(current_filename);
                    should_process = true;
                }
                 // pthread_mutex_unlock(&file_table_lock); // Example unlock
            }

            if (should_process) {
                char imagePath[1024];
                snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, current_filename);
                 // Lock if enqueue needs protection
                 pthread_mutex_lock(&queue_lock);
                 enqueue(&queue, imagePath);
                 pthread_mutex_unlock(&queue_lock);
            }
        }
        closedir(dir);
        dir = NULL; // Reset dir pointer

        // --- Check stop_flag again before sleeping ---
        if (stop_flag) {
             printf("Thread %lu: Stop flag detected after monitoring scan.\n", pthread_self());
             break; // Exit the outer monitoring loop
        }

        sleep(5); // Wait before checking again
    }

    // Ensure directory handle is closed if loop was exited due to stop_flag while dir was open
    if (dir != NULL) {
        closedir(dir);
    }

    printf("Thread %lu: Exiting gracefully.\n", pthread_self());
    return NULL; // Thread function returns, allowing join
}