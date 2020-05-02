#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/time.h>

#define MATCH(s) (!strcmp(argv[ac], (s)))
#define MAX_PLANES 500
#define LOG_FILE "planes2.log"
#define t 1

// given sleep function.
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

// function for keeping log of planes.
void log(int plane_id, char* status, int request_time, int runway_time, int turnaround_time){

    FILE* file;

    file = fopen(LOG_FILE, "a+");
    if(file == NULL){
        fprintf(stderr, "cannot open log file\n");
    }

    fprintf(file, " %d\t\t\t\t%s\t\t\t%d\t\t\t%d\t\t\t\t%d\n",plane_id,status,request_time,runway_time,turnaround_time);
    fclose(file);
}

// log the header of log file.
void log_header(){
    FILE* file;
    file = fopen(LOG_FILE, "w+");
    
    if(file == NULL){
        fprintf(stderr, "cannot open log file\n");
    }
    
    fprintf(file, "Plane ID    Status    Request Time   Runway Time   Turnaround Time\n------------------------------------------------------------------\n");
    fclose(file);
}

// ========= STRUCTS ===============
typedef struct Plane {
    int ID;
    time_t arrival_time;
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
// ======== END OF STRUCTS =========


// ========= QUEUE OPERATIONS ============
Queue* createQueue(int max_planes)
{
    Queue* queue;
    queue = (Queue*)malloc(sizeof(Queue));
    queue->capacity = max_planes;
    queue->size = 0;
    queue->front = 0;
    queue->rear = queue->capacity - 1;
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
};

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

Plane dequeue(Queue* queue)
{   
    Plane result;
    if(queue->size == 0)
    {
        printf("Queue is empty\n");
    }
    else
    {
        result = queue->planes[queue->front];
        queue->size = queue->size - 1;
        queue->front = queue->front + 1;
        if(queue->front == queue->capacity)
        {
            queue->front = 0;
        }
    }

    return result;
};

void print_queue(Queue* queue)
{
    if(queue->size == 0){
        printf("Empty queue!");
        return;
    }

    int first = queue->front;
    int last = queue->rear;
    while(first != last){
        printf("%d, ", queue->planes[first].ID);
        first = (first + 1) % queue->capacity;
    }

    printf("%d", queue->planes[last].ID);
    
};

// ============= END OF QUEUE OPERATIONS =============

// -------------- GLOBAL PARAMETERS ------------------
Queue *landing, *departing, *emergency;
Plane all_planes[MAX_PLANES];
pthread_mutex_t runway_mutex, start_mutex;
pthread_cond_t  runway_cond;
time_t start_time, end_time;
int simulation_time;
int emergency_check;
struct timespec ts;
struct timeval tv;



// -----------======== ESSENTIAL FUNCTIONS =========----------------
void *landing_func(void* ID)
{   
    // initialize plane
    int plane_id = (int)ID;
    Plane plane;
    plane.ID = plane_id;
    plane.arrival_time = time(NULL);
    pthread_mutex_init(&(plane.lock), NULL);
    pthread_cond_init(&(plane.cond), NULL);
    // add the plane to the global array so ATC can signal its cond.
    all_planes[plane_id] = plane;

    // acquire lock to update queue.
    pthread_mutex_lock(&runway_mutex);
    if(emergency_check){ // check for emergency
        enqueue(emergency, plane);
        emergency_check = 0; // reset emergency bool
    } else {
        enqueue(landing, plane);
    }
    // if this is the first plane, signal ATC to start working.
    if(plane_id == 1)
        pthread_cond_signal(&runway_cond);
    pthread_mutex_unlock(&runway_mutex);
    // wait for ATC signal to continue.
    pthread_cond_timedwait(&all_planes[plane_id].cond, &(all_planes[plane_id].lock), &ts);
    // release the lock and exit.
    pthread_mutex_unlock(&(all_planes[plane_id].lock));
    pthread_exit(NULL);
};

void *departing_func(void* ID)
{   
    // initialize the plane
    int plane_id = (int)ID;
    Plane plane;
    plane.ID = plane_id;
    plane.arrival_time = time(NULL);
    pthread_mutex_init(&(plane.lock), NULL);
    pthread_cond_init(&(plane.cond), NULL);
    // add the plane to a global array so that ATC can signal its cond.
    all_planes[plane_id] = plane;

    // acquire lock to update queue.
    pthread_mutex_lock(&runway_mutex);
    enqueue(departing, plane);
    // if this is the first plane, signal ATC to start working.
    if(plane_id == 1)
        pthread_cond_signal(&runway_cond);
    pthread_mutex_unlock(&runway_mutex);

    // wait for ATC signal to continue.
    pthread_cond_timedwait(&all_planes[plane_id].cond, &(all_planes[plane_id].lock), &ts);
    // release the lock and exit.
    pthread_mutex_unlock(&(all_planes[plane_id].lock));
    pthread_exit(NULL);
};

void *air_control()
{   
    // wait for signal from the first plane.
    pthread_cond_wait(&runway_cond, &start_mutex);
    int waiting_d, waiting_l;
    time_t current_time = time(NULL);
    while(current_time < end_time)
    {   
        // acquire lock to update queue (dequeue)
        pthread_mutex_lock(&runway_mutex);
        Plane plane; // plane to be dequeued
        // waiting time of the top plane in departing queue.
        //waiting_d = (departing->size > 0) ? (int)(current_time - top(departing).arrival_time) : 0;
        //waiting_l = (landing->size > 0) ? (int)(current_time - top(landing).arrival_time) : 0;
        time_t printing = time(NULL); // current time for log, called for being precise.
        if(emergency->size > 0){ // if there is a plane in emergency queue, give priority.
            plane = dequeue(emergency);
            log(plane.ID, "E", (int)(plane.arrival_time - start_time), (int)(printing - start_time), (int)(printing - plane.arrival_time));
        } else {
            // if less than 5 departing planes and waiting less than 10 seconds or more than 10 landing planes
            if((landing->size > 0 && departing->size < 5) || (landing->size >= 15)){
                plane = dequeue(landing);
                log(plane.ID, "L", (int)(plane.arrival_time - start_time), (int)(printing - start_time), (int)(printing - plane.arrival_time));
            // if there is no landing plane
            } else if((departing->size > 0 && landing->size < 15) || (departing->size >= 5)){
                plane = dequeue(departing);
                log(plane.ID, "D", (int)(plane.arrival_time - start_time), (int)(printing - start_time), (int)(printing - plane.arrival_time));
            }
        }
        // signal the plane after log its information.
        pthread_cond_signal(&(all_planes[plane.ID].cond));
        // release the lock and sleep for 2 seconds.
        pthread_mutex_unlock(&runway_mutex);
        pthread_sleep(2 * t);
        current_time = time(NULL);
    }
    pthread_mutex_unlock(&start_mutex);
    pthread_exit(NULL);
};

// -----------======== END OF ESSENTIAL FUNCTIONS =========------------

int main(int argc, char* argv[])
{
    float prob = 0.5; // landing plane probability, p.
    int seed = time(NULL); // random seed, initially set to the current time.
    int print = 25; // time to start printing queues on console.
    int max_planes = 100; // capacity for queues.
    
    if(argc < 2) {
	  printf("Usage: %s [-p <probability>] [-s <simulation_time>] [-seed <random_seed>] [-n <print_queue>]\n",argv[0]);
	  return(-1);
	}
	for(int ac = 1; ac < argc; ac++) {
		if(MATCH("-p")) {
			prob = atof(argv[++ac]);
		} else if(MATCH("-s")) {
			simulation_time = atoi(argv[++ac]);
        } else if(MATCH("-seed")) {
			seed = atoi(argv[++ac]);
        } else if(MATCH("-n")) {
			print = atoi(argv[++ac]);
		} else {
	        printf("Usage: %s [-p <probability>] [-s <simulation_time>] [-seed <random_seed>] [-n <print_queue>]\n",argv[0]);
		    return(-1);
		}
	}

    // initialize queues.
    max_planes = simulation_time * 2 + 2;
    emergency = createQueue(max_planes);
    landing = createQueue(max_planes);
    departing = createQueue(max_planes);

    // header of log file.
    log_header();
    // initialize mutex locks and conds.
    pthread_mutex_init(&runway_mutex, NULL);
    pthread_mutex_init(&start_mutex, NULL);
    pthread_cond_init(&runway_cond, NULL);

    // seed the random and get start time.
    srand(seed);
    time(&start_time);
    end_time = start_time + simulation_time;
    end_time++; // for the last iteration, add 1.

    // create place for plane threads and start ATC thread.
    pthread_t planes[max_planes];
    pthread_t tower;
    pthread_create(&tower, NULL, air_control, NULL);

    // for pthread_cond_timedwait timespec parameter
    gettimeofday(&tv, NULL);
    ts.tv_sec  = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    ts.tv_sec += simulation_time + 2 * t;

    // at time 0, there is one landing, one departing plane.
    int plane_id = 1;
    pthread_create(&(planes[plane_id]), NULL, departing_func, (void*)plane_id);
    plane_id++;
    pthread_create(&(planes[plane_id]), NULL, landing_func, (void*)plane_id);
    plane_id++;

    // since time 0 there are only two planes, start with time = 1
    pthread_sleep(t);

    // start the simulation clock
    time_t current_time = time(NULL);
    while(current_time < end_time)
    {   
        float r = (float)rand()/RAND_MAX;
        int time_passed = (int)(current_time - start_time);
        
        // start printing the queues on console
        if(time_passed >= print){
            printf("At %d sec:\nlanding: ", time_passed);
            print_queue(landing);
            printf("\n");
            printf("departing: ");
            print_queue(departing);
            printf("\n");
        }
        // check if the time for emergency plane comes.
        if(time_passed % 40 == 0 && time_passed > 0)
        {   
            // check variable for emergency queue
            emergency_check = 1;
            pthread_create(&(planes[plane_id]), NULL, landing_func, (void*)plane_id);
            plane_id++;
        } 
        else 
        {   
            // probability for landing plane
            if(r < prob){
                pthread_create(&(planes[plane_id]), NULL, landing_func, (void*)plane_id);
                plane_id++;
            }
            // probability for departing plane
            if(r < 1 - prob){
                pthread_create(&(planes[plane_id]), NULL, departing_func, (void*)plane_id);
                plane_id++;
            }
        }
        // wait for t seconds to continue
        pthread_sleep(t);
        current_time = time(NULL);
    }

    // join all plane threads.
    for(int i = 0; i < plane_id; i++){
        pthread_join(planes[i],NULL);
    }
    // join ATC thread
    pthread_join(tower,NULL);
    //remove mutex locks and cond.
    pthread_mutex_destroy(&runway_mutex);
    pthread_mutex_destroy(&start_mutex);
    pthread_cond_destroy(&runway_cond);
    // remove queues.
    free(departing);
    free(landing);
    free(emergency);

    return 0;
}