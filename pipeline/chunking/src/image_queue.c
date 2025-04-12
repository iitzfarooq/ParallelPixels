#include<image_queue.h>

image_queue_t queue = {NULL, NULL};
pthread_mutex_t queue_lock;

int enqueue(image_queue_t *const q, const char *const name) {
    queue_node_t *new_node = (queue_node_t*)malloc(sizeof(queue_node_t));
    if (new_node == NULL) {
        perror("enqueue - Failed to allocate memory for new queue node");
        return 1; 
    }
    new_node->next = NULL; 

    size_t name_len = strlen(name);
    new_node->name = (char*)malloc(name_len + 1);
    if (new_node->name == NULL) {
        perror("enqueue - Failed to allocate memory for node name");
        free(new_node); 
        return 1; 
    }
    strcpy(new_node->name, name); 

    if (q->end == NULL) {
        if (q->start != NULL) {
            fprintf(stderr, "enqueue: Queue inconsistency detected (end is NULL, start is not)\n");
            free(new_node->name);
            free(new_node);
            return 1; 
        }
        q->start = new_node;
        q->end = new_node;
    } else {
        q->end->next = new_node; 
        q->end = new_node;       
    }

    printf("image enqueued successfully\n");

    return 0; 
}

char* dequeue(image_queue_t *const q) {
    if (q->start == NULL) {
        if (q->end != NULL) 
            fprintf(stderr, "dequeue: Queue inconsistency detected (start is NULL, end is not)\n");

        else fprintf(stderr, "dequeue: Queue is empty.\n");

        return NULL; 
    }

    queue_node_t* dequeue_node = q->start;
    char* name = dequeue_node->name; 

    q->start = dequeue_node->next;

    if (q->start == NULL) {
        q->end = NULL;
    }

    free(dequeue_node);

    printf("image dequeued successfully\n");

    return name; 
}

void free_queue(image_queue_t *const q) {
    queue_node_t* curr = q->start;
    queue_node_t* temp;

    while(curr != NULL) {
        free(curr->name);
        temp = curr;
        curr = curr->next;
        free(temp);
    }

    q->start = NULL;
    q->end = NULL;
}