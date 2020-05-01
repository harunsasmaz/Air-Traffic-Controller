#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

//start of queue implementation
int sim;

void initQueue(Queue *q,int si){
    sim=si;
    q->queue = (plane*) malloc(si*sizeof(plane));
    q->size = 1;
    q->used = 0;
}

void enqueue(Queue *q, plane element){
      if (q->queue == NULL)
          initQueue(q,sim);

    if(q->size == 0) q->size = 1;

    (q->queue)[q->used++] = element;
}

plane dequeue(Queue *q){
    plane c;
    c.ID=111;
    if (q->size > 0 && q->used > 0){
        c = (q->queue)[0];
        int i;

        for (i=1; i<q->used; i++){
            q->queue[i-1] = q->queue[i];
        }

        q->used -= 1;

    }
    return c;
}

plane peekOfQueue(Queue q){
    plane c;
    c.ID = 1222;
    if (q.size > 0 && q.used > 0){
        c = (q.queue)[0];
    }
    return c;
}

int isQueueEmpty(Queue q){
    if (q.used == 0)
        return 1;
    else
        return 0;
}

void freeQueue(Queue *q){
    q->size = 1;
    q->used = 0;
    free(q->queue);
    q->queue = NULL;
}

void printQueue(Queue q){
  int i;
  for(i=0;i<q.used;i++){
    plane a = (q.queue)[i];
    if(q.used-1 == i )
      printf("%d",a.ID);
      else
    printf("%d,",a.ID);
  }
}

//end of queue implementation

//start of logging implementation
bool LogCreated = false;

void Log2 (int a,char *message,int b,int c,int d)
{
    FILE *file;

    if (!LogCreated) {
        file = fopen(LOGFILE, "w");
        LogCreated = true;
    }
    else
        file = fopen(LOGFILE, "a");

    if (file == NULL) {
        if (LogCreated)
            LogCreated = false;
        return;
    }
    else
    {
      if(a < 10){
          if(c < 10){
            fprintf(file, " %d         %s         %d            %d            %d\n",a,message,b,c,d);
          } else {
            fprintf(file, " %d         %s         %d            %d           %d\n",a,message,b,c,d);
          }
      } else{
          if(c< 10){
            fprintf(file, " %d        %s         %d            %d            %d\n",a,message,b,c,d);
          } else {
            if(b < 10){
              fprintf(file, " %d        %s         %d            %d           %d\n",a,message,b,c,d);
            } else {
              fprintf(file, " %d        %s         %d           %d           %d\n",a,message,b,c,d);
            }
        }
      }


    }

    if (file)
        fclose(file);
}


void Log1 (char *message)
{
    FILE *file;

    if (!LogCreated) {
        file = fopen(LOGFILE, "w");
        LogCreated = true;
    }
    else
        file = fopen(LOGFILE, "a");

    if (file == NULL) {
        if (LogCreated)
            LogCreated = false;
        return;
    }
    else
    {
        fprintf(file, "%s\n",message);
    }

    if (file)
        fclose(file);
}

//end of logging implementation

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

//locks,condition variables global, queues and time_t variables

Queue Landing,Departing,Emergency;
plane planesArray[200];
pthread_mutex_t plane_mutex;
pthread_cond_t  plane_cond;
int simulation,seed,n_sec,isEmergency;
float probability;
time_t currentTime,printer,departingPrinter,landingPrinter,endTime,turnaround;

struct timespec   ts;
struct timeval    tp;

// main function
int main(int argc,char **argv){
  int i;
  Log1("PlaneID   Status  Request Time   Runway Time  Turnaround Time\n-------------------------------------------------------");
  //getting arguments from command line
  // -s . simulationtime -n . every n second print queues -seed . seed for debugging -p . probability
  seed = 0;
  for(i=1;i<argc;i++){
      if(!strcmp(argv[i], "-p")) {
        probability = atof(argv[++i]);
      }
      else if(!strcmp(argv[i], "-s")) {
        simulation = atoi(argv[++i]);
      }
      else if(!strcmp(argv[i], "-n"))  {
        n_sec = atoi(argv[++i]);
      }
      else if(!strcmp(argv[i], "-seed"))  {
        seed = atoi(argv[++i]);
      }
  }

   // initializing queues

    initQueue(&Landing,simulation);
    initQueue(&Departing,simulation);
    initQueue(&Emergency,simulation);

  // initializing mutex,cond variables

    pthread_mutex_init(&plane_mutex, NULL);
    pthread_cond_init(&plane_cond, NULL);

    // feeding the seed and record the current time
    if(seed!= 0)
      srand(seed);
    time (&currentTime);

    int numberOfPlanes = simulation+2;
    pthread_t planes[numberOfPlanes];
    pthread_t ATC;
    pthread_create(&ATC,NULL,AirTraffic,NULL);

    // to make sure we run until the end of simulation
    endTime = currentTime + simulation;
    printer = time(NULL);
    endTime++;

    // here is for the pthread_cond_timedwait function
    gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += simulation+2;

    int planeID = 1;
    isEmergency = 0;
    pthread_create(&planes[planeID],NULL,LandingPlane,(void *) planeID);
    planeID++;
    pthread_create(&planes[planeID],NULL,DepartingPlane,(void *) planeID);
    planeID++;

    // until we reach simulation + start time continue creating planes

      while(printer < endTime){
        float random = (float)rand()/RAND_MAX;
        int timePassed = ((int) printer-currentTime );

        // printing to terminal every n seconds
        if(timePassed % n_sec == 0 && timePassed > 0 ){

          printf("\nAt %d sec air: ",timePassed);
          printQueue(Landing);
          printf("\n");
          printf("At %d sec ground: ",timePassed);
          printQueue(Departing);
          printf("\n");

        }

        // PART III emergency
        if(timePassed % 39 == 0 && timePassed > 0){
          //emergency lane
          printf("EMERGENCY QUEUE\n");
          pthread_create(&planes[planeID],NULL,LandingPlane,(void *) planeID);
          planeID++;
          isEmergency = 1;
        } else{

          // create landing or departing plane according to the probability

        if(random < probability){
          pthread_create(&planes[planeID],NULL,LandingPlane,(void *) planeID);
          planeID++;
        } else{
          pthread_create(&planes[planeID],NULL,DepartingPlane,(void *) planeID);
          planeID++;
        }
      }

        printer = time(NULL);
        pthread_sleep(1);
        //printf("loop time is : %s and float is : %0.2f", ctime(&printer),random);
  }

  // join threads
        for(i = 0; i<planeID;i++){
          pthread_join(planes[i],NULL);
        }
          pthread_join(ATC,NULL);

        // destroy mutex and condition variables
        pthread_mutex_destroy(&plane_mutex);
        pthread_cond_destroy(&plane_cond);

        freeQueue(&Landing);
        freeQueue(&Departing);
        freeQueue(&Emergency);

    return 0;
}

// function for landing planes

void* LandingPlane(void *ID){
  int planeID = (int) ID;
  plane temp;
  temp.ID = planeID;
  temp.requestTime = time(NULL);
  pthread_mutex_init(&(temp.mutex), NULL);
  pthread_cond_init(&(temp.cond), NULL);
  planesArray[planeID] = temp;
  int emergency_id;

  // get the lock from the airTrafficControl
  pthread_mutex_lock(&plane_mutex);

  pthread_mutex_lock(&planesArray[planeID].mutex);

  // add the plane to the queue

  if(isEmergency == 1)
    enqueue(&Emergency,temp);
  else
    enqueue(&Landing,temp);

  // wait for the specific plane to be signalled

  pthread_cond_timedwait(&(planesArray[planeID].cond),&plane_mutex,&ts);

  // add the plane to the log

  landingPrinter = time(NULL);
  if(landingPrinter < endTime){
    if(isEmergency == 1){
      Log2(temp.ID,"E",(temp.requestTime-currentTime),(int)(landingPrinter-currentTime),(int)(landingPrinter-temp.requestTime));
      emergency_id = temp.ID;
      isEmergency = 0;
    }
    if(temp.ID != emergency_id)
    Log2(temp.ID,"L",(temp.requestTime-currentTime),(int)(landingPrinter-currentTime),(int)(landingPrinter-temp.requestTime));
  }

  pthread_mutex_unlock(&planesArray[planeID].mutex);

  pthread_mutex_unlock(&plane_mutex);
  pthread_exit(NULL);
}

// same as landingplane function except we don't have emergency departures

void* DepartingPlane(void *ID){
  int planeID = (int) ID;
  plane temp;
  temp.ID = planeID;
  temp.requestTime = time(NULL);
  pthread_mutex_init(&(temp.mutex), NULL);
  pthread_cond_init(&(temp.cond), NULL);
  planesArray[planeID] = temp;

  pthread_mutex_lock(&plane_mutex);

  pthread_mutex_lock(&planesArray[planeID].mutex);
  enqueue(&Departing,temp);

  pthread_cond_timedwait(&(planesArray[planeID].cond),&plane_mutex,&ts);
  departingPrinter = time(NULL);
  if(departingPrinter < endTime){
    Log2(temp.ID,"D",(temp.requestTime-currentTime),(int)(departingPrinter-currentTime),(int)(departingPrinter-temp.requestTime));
  }

  pthread_mutex_unlock(&planesArray[planeID].mutex);

  pthread_mutex_unlock(&plane_mutex);
  pthread_exit(NULL);
}

// ATC function

void* AirTraffic(void *ID){
  time_t printer,endTime;
  endTime = currentTime + simulation;
  printer = time(NULL);
  int waitingTime;

  // continue until the end of simulation time

    while(printer < endTime){
      pthread_mutex_lock(&plane_mutex);
      if(isQueueEmpty(Departing) == 0){
        plane printing = peekOfQueue(Departing);
        waitingTime = (int)(printer - printing.requestTime);
      } else waitingTime = 0;

  // emergency plane present so signal immediately

    if(isEmergency == 1 && isQueueEmpty(Emergency) == 0){
      plane temp = dequeue(&Emergency);
      pthread_cond_signal(&(planesArray[temp.ID].cond));
    } else {

    // if Landing queue is not empty OR number of planes waiting to depart < 3 or waiting time is < 10, pop and land
      if((isQueueEmpty(Landing) == 0 && waitingTime < 10 && Departing.used < 3) || (isQueueEmpty(Landing) == 0 && Landing.used > 8)){

        plane temp = dequeue(&Landing);
        pthread_cond_signal(&(planesArray[temp.ID].cond));

      } else if(isQueueEmpty(Landing) == 1 && isQueueEmpty(Departing) == 0){
        // landing queue is empty so allow 1 departing plane
          plane temp = dequeue(&Departing);
          pthread_cond_signal(&(planesArray[temp.ID].cond));
        // if the plane in front of the departing queue has been waiting for over 10 seconds or departing queue has more than 3
        // planes lined up, signal one plane to depart.
      } else if ((waitingTime >= 10 && isQueueEmpty(Departing) == 0) || (isQueueEmpty(Departing) == 0 && Departing.used >= 3)){
          plane temp = dequeue(&Departing);
          pthread_cond_signal(&(planesArray[temp.ID].cond));
      }
  }

    printer = time(NULL);
    pthread_mutex_unlock(&plane_mutex);
    pthread_sleep(2);

  }
  pthread_exit(NULL);
}
// end of the project
