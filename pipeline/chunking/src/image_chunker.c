#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chunker_header_all.h>
#include <image_chunker.h>
#include <math.h>
#include <image.h> 

unsigned char *load_image(const char *filename, int *width, int *height, int *channels) {
    unsigned char *data = stbi_load(filename, width, height, channels, 0);
    if (data == NULL) {
        fprintf(stderr, "Error loading image '%s': %s\n", filename, stbi_failure_reason());
        return NULL;
    }

    printf("Loaded image '%s' (%d x %d, %d channels)\n", filename, *width, *height, *channels);
    return data;
}

static image_chunk_t **create_chunks_internal(const char *original_filename,
                                              unsigned char *image_data,
                                              int width, int height, int channels,
                                              int chunk_width, int chunk_height,
                                              int *num_chunks_total)
{
    // --- Input validation ---
    if (!image_data || width <= 0 || height <= 0 || channels <= 0 || chunk_width <= 0 || chunk_height <= 0) {
        fprintf(stderr, "create_chunks_internal: Invalid input parameters.\n");
        return NULL;
    }

    // --- Calculate chunk grid dimensions ---
    int num_chunks_x = (int)ceil((double)width / chunk_width);
    int num_chunks_y = (int)ceil((double)height / chunk_height);
    *num_chunks_total = num_chunks_x * num_chunks_y;

    if (*num_chunks_total <= 0) {
         fprintf(stderr, "create_chunks_internal: Calculated zero chunks.\n");
         return NULL;
    }

    image_chunk_t **chunks = (image_chunk_t **)malloc(*num_chunks_total * sizeof(image_chunk_t *));
    if (chunks == NULL) {
        perror("create_chunks_internal: Failed to allocate memory for chunk array");
        return NULL;
    }

    for(int i = 0; i < *num_chunks_total; ++i) 
        chunks[i] = NULL;

    int current_chunk_index = 0;
    size_t bytes_per_pixel = channels;

    for (int cy = 0; cy < num_chunks_y; ++cy) {
        for (int cx = 0; cx < num_chunks_x; ++cx) {
            // Allocate chunk struct
            chunks[current_chunk_index] = (image_chunk_t*)malloc(sizeof(image_chunk_t));
            if (chunks[current_chunk_index] == NULL) {
                perror("create_chunks_internal: Failed to allocate memory for chunk struct");
                free_image_chunks(chunks, current_chunk_index); 
                return NULL;
            }

            image_chunk_t *chunk = chunks[current_chunk_index];

            chunk->offset_x = cx * chunk_width;
            chunk->offset_y = cy * chunk_height;
            chunk->width = (chunk->offset_x + chunk_width > width) ? (width - chunk->offset_x) : chunk_width;
            chunk->height = (chunk->offset_y + chunk_height > height) ? (height - chunk->offset_y) : chunk_height;
            chunk->channels = channels;
            chunk->chunk_id = current_chunk_index;

            size_t name_len = strlen(original_filename);
            chunk->original_image_name = (char*)malloc(name_len + 1);
            if (chunk->original_image_name == NULL) {
                perror("create_chunks_internal: Failed to allocate memory for chunk filename");
                free(chunk);
                chunks[current_chunk_index] = NULL;
                free_image_chunks(chunks, current_chunk_index);
                return NULL;
            }
            
            strcpy(chunk->original_image_name, original_filename);

            chunk->data_size_bytes = chunk->width * chunk->height * bytes_per_pixel;
            chunk->pixel_data = (unsigned char*)malloc(chunk->data_size_bytes);
            if (chunk->pixel_data == NULL) {
                perror("create_chunks_internal: Failed to allocate memory for chunk pixel data");
                free(chunk->original_image_name);
                free(chunk);
                chunks[current_chunk_index] = NULL;
                free_image_chunks(chunks, current_chunk_index);
                return NULL; 
            }

            size_t src_row_stride = width * bytes_per_pixel;
            size_t chunk_row_stride = chunk->width * bytes_per_pixel;
            for (size_t row = 0; row < chunk->height; ++row) {
                unsigned char *src_ptr = image_data + (chunk->offset_y + row) * src_row_stride + chunk->offset_x * bytes_per_pixel;
                unsigned char *dest_ptr = chunk->pixel_data + row * chunk_row_stride;
                memcpy(dest_ptr, src_ptr, chunk_row_stride);
            }

            current_chunk_index++;
        }
    }

    return chunks;
}


void *chunk_image_thread(void *arg) {
    chunker_input_t *input = (chunker_input_t *)arg;

    chunker_output_t *output = (chunker_output_t *)malloc(sizeof(chunker_output_t));
    if (output == NULL) {
        perror("chunk_image_thread: Failed to allocate memory for output struct");
        return NULL;
    }
    output->chunks = NULL;
    output->num_chunks_total = 0;
    output->error_flag = 1; 

    printf("Chunker thread started for %s.\n", input->original_filename);

    output->chunks = create_chunks_internal(
        input->original_filename,
        input->image_data,
        input->width, input->height, input->channels,
        input->chunk_width, input->chunk_height,
        &output->num_chunks_total
    );

    if (output->chunks != NULL) {
        output->error_flag = 0;
        printf("Chunker thread finished successfully creating %d chunks for %s.\n",
               output->num_chunks_total, input->original_filename);
    } else {
        output->error_flag = 1; 
        fprintf(stderr, "Chunker thread failed for %s.\n", input->original_filename);
        output->num_chunks_total = 0;
    }

    return (void *)output;
}

// --- Cleanup (Remains the same) ---
void free_image_chunks(image_chunk_t **chunks, int num_chunks) {
    if (chunks == NULL) {
        return;
    }
    for (int i = 0; i < num_chunks; ++i) {
        if (chunks[i] != NULL) {
            free(chunks[i]->original_image_name);
            free(chunks[i]->pixel_data);
            free(chunks[i]);
        }
    }
    free(chunks);
}