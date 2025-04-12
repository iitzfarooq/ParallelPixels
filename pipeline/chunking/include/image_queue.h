#pragma once

#include<chunker_header_all.h>

struct queue_node;

typedef struct queue_node {
    struct queue_node* next;
    char* name;
} queue_node_t;

typedef struct {
    queue_node_t* start;
    queue_node_t* end;
} image_queue_t;

extern image_queue_t queue;
extern pthread_mutex_t queue_lock;

int enqueue(image_queue_t *const q, const char *const name);
char* dequeue(image_queue_t *const q);
void free_queue(image_queue_t*);