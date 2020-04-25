#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/time.h>

#define MATCH(s) (!strcmp(argv[ac], (s)))
#define MAX_PLANES 500
#define LOG_FILE "airport.log"
#define t 1


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

void log(int plane_id, char* status, int request_time, int runway_time, int turnaround_time){

    FILE* file;

    file = fopen(LOG_FILE, "a+");
    if(file == NULL){
        fprintf(stderr, "cannot open log file\n");
    }

    fprintf(file, " %d\t\t\t%s\t\t\t%d\t\t\t%d\t\t\t%d\n",plane_id,status,request_time,runway_time,turnaround_time);
    fclose(file);
}

void log_header(){
    FILE* file;
    file = fopen(LOG_FILE, "w+");
    
    if(file == NULL){
        fprintf(stderr, "cannot open log file\n");
    }
    
    fprintf(file, "Plane ID    Status    Request Time   Runway Time   Turnaround Time\n------------------------------------------------------------------\n");
    fclose(file);
}

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

int dequeue(Queue* queue)
{   
    int result = -1;
    if(queue->size == 0)
    {
        printf("Queue is empty\n");
    }
    else
    {
        result = queue->planes[queue->front].ID;
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
    
};

Queue *landing, *departing, *emergency;
Plane all_planes[MAX_PLANES];
pthread_mutex_t runway_mutex;
pthread_cond_t  runway_cond;
time_t start_time, end_time;
int simulation_time;
int emergency_check;
struct timespec ts;
struct timeval tv;

void *landing_func(void* ID)
{   
    int plane_id = (int)ID;
    Plane plane;
    plane.ID = plane_id;
    plane.arrival_time = time(NULL);
    pthread_mutex_init(&(plane.lock), NULL);
    pthread_cond_init(&(plane.cond), NULL);
    all_planes[plane_id] = plane;


    pthread_mutex_lock(&runway_mutex);
    pthread_mutex_lock(&(all_planes[plane_id].lock));

    
    if(emergency_check)
        enqueue(emergency, plane);
    else
        enqueue(landing, plane);

    pthread_cond_timedwait(&all_planes[plane_id].cond, &runway_mutex, &ts);

    time_t landing_log = time(NULL);
    if(landing_log < end_time){
        char* status = (emergency_check == 1) ? "E" : "L";
        log(plane_id, status, (int)(plane.arrival_time - start_time), (int)(landing_log - start_time), (int)(landing_log - plane.arrival_time));
        emergency_check = 0;
    }

    pthread_mutex_unlock(&(all_planes[plane_id].lock));
    pthread_mutex_unlock(&runway_mutex);
    pthread_exit(NULL);
};

void *departing_func(void* ID)
{   
    int plane_id = (int)ID;
    Plane plane;
    plane.ID = plane_id;
    plane.arrival_time = time(NULL);
    pthread_mutex_init(&(plane.lock), NULL);
    pthread_cond_init(&(plane.cond), NULL);
    all_planes[plane_id] = plane;

    
    pthread_mutex_lock(&runway_mutex);
    pthread_mutex_lock(&(all_planes[plane_id].lock));

    enqueue(departing, plane);

    pthread_cond_timedwait(&all_planes[plane_id].cond, &runway_mutex, &ts);

    time_t departing_log = time(NULL);
    if(departing_log < end_time)
        log(plane_id, "D", (int)(plane.arrival_time - start_time), (int)(departing_log - start_time), (int)(departing_log - plane.arrival_time));

    pthread_mutex_unlock(&(all_planes[plane_id].lock));
    pthread_mutex_unlock(&runway_mutex);
    pthread_exit(NULL);
};

void *air_control()
{   
    time_t current_time = time(NULL);
    while(current_time < end_time)
    {   
        pthread_mutex_lock(&runway_mutex);
        int waiting = (departing->size > 0) ? (int)(current_time - top(departing).arrival_time) : 0;

        if(emergency->size > 0){
            int id = dequeue(emergency);
            pthread_cond_signal(&all_planes[id].cond);
        } else {

            if((landing->size > 0 && waiting < 10 && departing->size < 3) || (landing->size > 0 && landing->size > 7)){
                int id = dequeue(landing);
                pthread_cond_signal(&all_planes[id].cond);
            } else if(landing->size == 0 && departing->size > 0){
                int id = dequeue(departing);
                pthread_cond_signal(&all_planes[id].cond);
            } else if((departing->size > 0 && waiting >= 10) || (departing->size > 0 && departing->size >= 3)){
                int id = dequeue(departing);
                pthread_cond_signal(&all_planes[id].cond);
            }
            
        }

        current_time = time(NULL);
        pthread_mutex_unlock(&runway_mutex);
        pthread_sleep(2 * t);
    }
    
    pthread_exit(NULL);
};

int main(int argc, char* argv[])
{
    float prob = 0.5;
    int seed = 1;
    int print = 25;
    int max_planes = 100;
    
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

    //TODO: init mutexes and conds
    max_planes = simulation_time + 2;
    emergency = createQueue(max_planes);
    landing = createQueue(max_planes);
    departing = createQueue(max_planes);

    log_header();

    pthread_mutex_init(&runway_mutex, NULL);
    pthread_cond_init(&runway_cond, NULL);

    srand(seed);
    time(&start_time);
    end_time = start_time + simulation_time;
    end_time += 2;

    pthread_t planes[max_planes];
    pthread_t tower;
    pthread_create(&tower, NULL, air_control, NULL);

    gettimeofday(&tv, NULL);
    ts.tv_sec  = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    ts.tv_sec += simulation_time + 2 * t;

    time_t current_time = time(NULL);

    int plane_id = 1;
    pthread_create(&(planes[plane_id]), NULL, departing_func, (void*)plane_id);
    plane_id++;
    pthread_create(&(planes[plane_id]), NULL, landing_func, (void*)plane_id);
    plane_id++;

    while(current_time < end_time)
    {   
        float r = (float)rand()/RAND_MAX;
        int time_passed = (int)(current_time - start_time);

        if(time_passed % print == 0 && time_passed > 0){
            printf("\nAt %d sec landing: ", time_passed);
            print_queue(landing);
            printf("\n");
            printf("At %d sec departing: ", time_passed);
            print_queue(departing);
            printf("\n");
        }

        if(time_passed % 39 == 0 && time_passed > 0)
        {
            emergency_check = 1;
            pthread_create(&(planes[plane_id]), NULL, landing_func, (void*)plane_id);
            plane_id++;
        } 
        else 
        {
            if(r < prob){
                pthread_create(&(planes[plane_id]), NULL, landing_func, (void*)plane_id);
                plane_id++;
            }
            else {
                pthread_create(&(planes[plane_id]), NULL, departing_func, (void*)plane_id);
                plane_id++;
            }
        }
        
        current_time = time(NULL);
        pthread_sleep(t);
    }

    for(int i = 0; i < plane_id; i++){
        pthread_join(planes[i],NULL);
    }

    pthread_join(tower,NULL);

    pthread_mutex_destroy(&runway_mutex);
    pthread_cond_destroy(&runway_cond);

    free(departing);
    free(landing);
    free(emergency);

    return 0;
}