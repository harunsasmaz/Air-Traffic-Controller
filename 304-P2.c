#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>

static int plane_id = 0;

typedef struct Plane {
    int ID;
    int arrival_time;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
}Plane;

typedef struct Queue {

    int capacity;
    int size;
    int rear;
    int front;
    struct Plane *planes;
}Queue;

Plane* createPlane(int arrival_time){

    Plane* plane;
    plane = (Plane*)malloc(sizeof(Plane));
    plane->arrival_time = arrival_time;
    plane->ID = ++plane_id;
    pthread_mutex_init(&plane->lock, NULL);
    pthread_cond_init(&plane->cond, NULL);

    return plane;
}

Queue* createQueue(int max_planes)
{
    Queue* queue;
    queue = (Queue*)malloc(sizeof(Queue));
    queue->capacity = max_planes;
    queue->size = 0;
    queue->front = 0;
    queue->rear = -1;
    queue->planes = (Plane*) malloc(max_planes * sizeof(Plane));

    return queue;
}

void enqueue(Queue *queue, Plane plane)
{
    if(queue->size == queue->capacity)
    {
            printf("Queue is Full\n");
    }
    else
    {
            queue->size++;
            queue->rear = queue->rear + 1;
            if(queue->rear == queue->capacity)
            {
                    queue->rear = 0;
            }
            queue->elements[queue->rear] = plane;
    }
}

void dequeue(struct Queue *queue)
{
    if(queue->size==0)
    {
        printf("Queue is Empty\n");
    }
    else
    {
        queue->size--;
        queue->front++;
        if(queue->front == queue->capacity)
        {
            queue->front=0;
        }
    }
}
