#include"uthash.h"

typedef void (*char_process_function_ptr)(const char*);
typedef void (*void_process_function_ptr)(void);

// Structure for the hash table entries
typedef struct {
    char name[256]; // Filename (adjust size if needed)
    UT_hash_handle hh; // Makes this structure hashable
    char_process_function_ptr add_processed_file;
    char_process_function_ptr was_file_processed;
    void_process_function_ptr free_processed_files;
} processed_file_t;