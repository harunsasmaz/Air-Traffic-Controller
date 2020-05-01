#include <time.h>
#include <pthread.h>

#define LOGFILE	"planes.log"
void Log2 (int a,char *message,int b,int c,int d);
void Log1 (char *message);
void LogErr (char *message);

typedef struct{
    int ID;
    time_t requestTime;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} plane;

typedef struct {
    plane *queue;
    int used;
    int size;
} Queue;

void initQueue(Queue *q,int si);
void enqueue(Queue *q, plane element);
plane dequeue(Queue *q);
int isQueueEmpty(Queue q);
plane peekOfQueue(Queue q);
void freeQueue(Queue *q);

int pthread_sleep (int seconds);
void* LandingPlane(void *ID);
void* DepartingPlane(void *ID);
void* AirTraffic(void *ID);
