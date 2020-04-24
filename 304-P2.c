#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/time.h>

#define MATCH(s) (!strcmp(argv[ac], (s)))
#define MAX_PLANES 100
#define t 1

int simulation_time = 100;
double start_time = 0;
static int plane_id = 0;

int pthread_sleep (int seconds)
{
   pthread_mutex_t mutex;
   pthread_cond_t conditionvar;
   struct timespec timetoexpire;
   if(pthread_mutex_init(&mutex,NULL))
    {
      return -1;
    }
   if(pthread_cond_init(&conditionvar,NULL))
    {
      return -1;
    }
   struct timeval tp;
   //When to expire is an absolute time, so get the current time and add //it to our delay time
   gettimeofday(&tp, NULL);
   timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

   pthread_mutex_lock (&mutex);
   int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
   pthread_mutex_unlock (&mutex);
   pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&conditionvar);

   //Upon successful completion, a value of zero shall be returned
   return res;

}

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

Plane top(Queue* queue)
{
    if(queue->size == 0)
    {
        printf("Queue is Empty\n");
        exit(0);
    }
    return queue->planes[queue->front];
}

void enqueue(Queue* queue, Plane plane)
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

void dequeue(Queue* queue)
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

Queue *landing, *departing, *emergency;

void *landing_func()
{
    Plane plane;
    plane.ID = ++plane_id;
    //TODO: init arrival time
    pthread_mutex_init(&plane.lock, NULL);
    pthread_cond_init(&plane.cond, NULL);
    

    pthread_exit(NULL);
    return NULL;
};

void *departing_func()
{
    Plane plane;
    plane.ID = ++plane_id;
    //TODO: init departing time
    pthread_mutex_init(&plane.lock, NULL);
    pthread_cond_init(&plane.cond, NULL);
    enqueue(departing, plane);
    if(plane.ID == departing->planes[departing->front].ID){}
        //TODO: notify ATC thread
    //TODO: wait for signal

    pthread_exit(NULL);
    return NULL;
};

void *air_control()
{
    //TODO: wait for signal

    while(get_time() < start_time + simulation_time)
    {
        if(emergency->size > 0){
            dequeue(emergency);
            pthread_sleep(t);
        } else if(departing->size > 0){ // the starvation condition will be added
            dequeue(departing);
            pthread_sleep(t);
        } else {
            dequeue(landing);
            pthread_sleep(t);
        }
    }

    pthread_exit(NULL);
    return NULL;
};



int main(int argc, char* argv[])
{
    float prob = 0.5;
    int max_planes = 100;
    
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

    //TODO: init mutexes and conds

    emergency = createQueue(max_planes);
    landing = createQueue(max_planes);
    departing = createQueue(max_planes);

    pthread_t tid[max_planes];

    pthread_create(&(tid[0]), NULL, air_control, NULL);
    pthread_create(&(tid[1]), NULL, landing_func, NULL);
    pthread_create(&(tid[2]), NULL, departing_func, NULL);

    start_time = get_time();
 
    int i = 3;
    while(get_time() < start_time + simulation_time)
    {
        double r = (double)rand() / (double)RAND_MAX;
        if(r <= prob){
            pthread_create(&(tid[i]), NULL, landing_func, NULL);
            i++;
        }
        if(r <= 1 - prob){
            pthread_create(&(tid[i]), NULL, departing_func, NULL);
            i++;
        }
        
        pthread_sleep(t);
    }

    return 0;
}