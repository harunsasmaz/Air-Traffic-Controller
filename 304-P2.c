#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/time.h>

#define MATCH(s) (!strcmp(argv[ac], (s)))

static int plane_id = 0;

static const double kMicro = 1.0e-6;
double get_time() {
	struct timeval TV;
	struct timezone TZ;
	const int RC = gettimeofday(&TV, &TZ);
	if(RC == -1) {
		printf("ERROR: Bad call to gettimeofday\n");
		return(-1);
	}
	return( ((double)TV.tv_sec) + kMicro * ((double)TV.tv_usec) );
}

typedef struct Plane {
    int ID;
    int arrival_time;
    pthread_mutex_t lock;
    pthread_cond_t cond;
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
};

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
};

void enqueue(Queue *queue, Plane plane)
{
    if(queue->size == queue->capacity)
    {
            printf("Queue is full\n");
    }
    else
    {
            queue->size++;
            queue->rear = queue->rear + 1;
            if(queue->rear == queue->capacity)
            {
                    queue->rear = 0;
            }
            queue->planes[queue->rear] = plane;
    }
};

void dequeue(struct Queue *queue)
{
    if(queue->size == 0)
    {
        printf("Queue is empty\n");
    }
    else
    {
        queue->size--;
        queue->front++;
        if(queue->front == queue->capacity)
        {
            queue->front = 0;
        }
    }
};

void landing()
{

};

void departing()
{

};

void air_control()
{

};



int main(int argc, char* argv[])
{
    float prob = 0.5;
    int simulation_time = 100;
    
    if(argc<2) {
	  printf("Usage: %s [-i < filename>] [-iter <n_iter>] [-l <lambda>] [-o <outputfilename>]\n",argv[0]);
	  return(-1);
	}
	for(int ac=1;ac<argc;ac++) {
		if(MATCH("-p")) {
			prob = atof(argv[++ac]);
		} else if(MATCH("-s")) {
			simulation_time = atoi(argv[++ac]);
		} else {
		    printf("Usage: %s [-s <simulation time>] [-p <probability>]\n",argv[0]);
		    return(-1);
		}
	}


}