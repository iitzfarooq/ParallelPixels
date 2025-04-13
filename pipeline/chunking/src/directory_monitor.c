#include<stdio.h> 
#include<dirent.h>         
#include<string.h>         
#include<unistd.h>        
#include<pthread.h>    
#include<stdbool.h>      
#include<signal.h>        
#include<stdlib.h>     
#include<directory_monitor.h>
#include<image_queue.h>       

extern volatile sig_atomic_t stop_flag; 
extern image_name_queue_t name_queue;

void *read_images_from_directory(void *arg) {
    const char *directoryPath = (const char *)arg;
    struct dirent *entry;
    DIR *dir = NULL;

    // --- Initial Scan ---
    printf("Thread %lu: Performing initial scan of directory: %s\n", pthread_self(), directoryPath);
    dir = opendir(directoryPath);
    if (dir == NULL) {
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "Thread %lu: Unable to open directory for initial scan (%s)", pthread_self(), directoryPath);
        perror(err_msg);
        return NULL; 
    }

    while ((entry = readdir(dir)) != NULL) {
        if (stop_flag) {
            printf("Thread %lu: Stop flag detected during initial scan.\n", pthread_self());
            break; 
        }

        bool should_process = false;
        char current_filename[256];
        strncpy(current_filename, entry->d_name, sizeof(current_filename) - 1);
        current_filename[sizeof(current_filename) - 1] = '\0';

        if (strstr(current_filename, ".jpg") || strstr(current_filename, ".png")) {
            if (!was_file_processed(current_filename)) {
                add_processed_file(current_filename);
                should_process = true;
            }
        }

        if (should_process) {
            char imagePath[1024];
            snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, current_filename);
            if (enqueue_image_name(&name_queue, imagePath) != 0) 
                fprintf(stderr, "Thread %lu: Failed to enqueue image %s\n", pthread_self(), imagePath);
        }
    }

    closedir(dir);
    dir = NULL; 
    if (stop_flag) {
        printf("Thread %lu: Stop flag detected after initial scan.\n", pthread_self());
        return NULL; 
    }

    printf("Thread %lu: Initial scan complete. Monitoring...\n", pthread_self());

    // --- Monitoring Loop ---
    while (!stop_flag) { 
        dir = opendir(directoryPath);
        if (dir == NULL) {
            char err_msg[512];
            snprintf(err_msg, sizeof(err_msg), "Thread %lu: Unable to open directory for monitoring (%s)", pthread_self(), directoryPath);
            perror(err_msg);
            if (stop_flag) break;
            sleep(5); 
            continue; 
        }

        while ((entry = readdir(dir)) != NULL) {
            if (stop_flag) {
                printf("Thread %lu: Stop flag detected during monitoring scan.\n", pthread_self());
                break; 
            }

            bool should_process = false;
            char current_filename[256];
            strncpy(current_filename, entry->d_name, sizeof(current_filename) - 1);
            current_filename[sizeof(current_filename) - 1] = '\0';

            if (strstr(current_filename, ".jpg") || strstr(current_filename, ".png")) {
                if (!was_file_processed(current_filename)) {
                    add_processed_file(current_filename);
                    should_process = true;
                }
            }

            if (should_process) {
                char imagePath[1024];
                snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, current_filename);
                if (enqueue_image_name(&name_queue, imagePath) != 0) {
                    fprintf(stderr, "Thread %lu: Failed to enqueue image %s\n", pthread_self(), imagePath);
                }
            }
        }
        closedir(dir);
        dir = NULL; 

        if (stop_flag) {
            printf("Thread %lu: Stop flag detected after monitoring scan.\n", pthread_self());
            break;
        }

        sleep(5); 
    }

    if (dir != NULL)
        closedir(dir);

    printf("Thread %lu: Exiting gracefully.\n", pthread_self());
    return NULL;
}