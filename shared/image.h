#include<ctype.h>

typedef struct {
    int id;
    char* name;
    int start_x;
    int start_y;
    int end_x;
    int end_y;

    size_t height;
    size_t width;

} image_chunk_t;