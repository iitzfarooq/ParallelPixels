#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<dirent.h>
#include<string.h>
#include<stdbool.h>
#include<signal.h>
#include<uthash.h> 
#include<image_table.h>

void processImage(const char *imagePath) {
    // Placeholder for image processing logic
    printf("Processing image: %s\n", imagePath);
    // Simulate some work
    usleep(50000); // 50ms
}

void *readImagesFromDirectory(void *arg) {
    const char *directoryPath = (const char *)arg;
    struct dirent *entry;
    DIR *dir;

    // Initial scan to process existing files and populate hash table
    printf("Performing initial scan of directory: %s\n", directoryPath);
    dir = opendir(directoryPath);
    if (dir == NULL) {
        perror("Unable to open directory for initial scan");
        return NULL;
    }
    while ((entry = readdir(dir)) != NULL) {
        // Check extension and if not already processed (shouldn't be on first scan, but good practice)
        if ((strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".png") || strstr(entry->d_name, ".bmp")) &&
            !was_file_processed(entry->d_name))
        {
            char imagePath[1024];
            snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, entry->d_name);
            processImage(imagePath);
            add_processed_file(entry->d_name); // Add to processed list
        }
    }
    closedir(dir);
    printf("Initial scan complete. Monitoring for new files...\n");


    while (true) { // Infinite loop to continuously check for *new* images
        dir = opendir(directoryPath);
        if (dir == NULL) {
            perror("Unable to open directory");
            // Consider adding a delay and retry instead of exiting the thread
            sleep(5);
            continue;
        }

        while ((entry = readdir(dir)) != NULL) {
            // Check extension AND if it's NOT already processed
            if ((strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".png") || strstr(entry->d_name, ".bmp")) &&
                !was_file_processed(entry->d_name))
            {
                char imagePath[1024];
                snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, entry->d_name);
                processImage(imagePath);
                add_processed_file(entry->d_name); // Add to processed list
            }
        }

        closedir(dir);
        sleep(5); // Wait before checking again
    }

    // Cleanup (won't be reached in this infinite loop, but good practice)
    // free_processed_files(); // Ideally called from main or signal handler
    return NULL;
}

void* exitHandler(int signum) {
    printf("Freeing Processed files");
    free_processed_files();
    printf("Cleaup done\nExiting.");
    exit(0);
}

__sighandler_t handleExit = (__sighandler_t)exitHandler;

// TODO: Add signal handling (like in the Python script) to call free_processed_files() on exit.

int main(void) {
    pthread_t thread;
    const char *directoryPath = "../images"; 

    // Optional: Create the directory if it doesn't exist
    // mkdir(directoryPath, 0777); // #include <sys/stat.h> for mkdir

    if (signal(SIGINT, handleExit) == SIG_ERR) {
        perror("Exit signal handler not assigned");
        return EXIT_FAILURE;
    }

    printf("Starting image processing thread for directory: %s\n", directoryPath);
    if (pthread_create(&thread, NULL, readImagesFromDirectory, (void *)directoryPath) != 0) {
        perror("Failed to create thread");
        free_processed_files(); // Clean up if thread creation failed
        return EXIT_FAILURE;
    }

    // Wait for the thread to finish (it won't in this example due to infinite loop)
    // In a real application, you might wait for a signal or other termination condition
    pthread_join(thread, NULL);

    // Cleanup hash table memory (ideally done via signal handler for Ctrl+C)
/*     printf("Cleaning up processed files hash table...\n");
    free_processed_files();
    printf("Cleanup complete.\n"); */

    return 0;
}