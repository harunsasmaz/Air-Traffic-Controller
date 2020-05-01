# Air Traffic Control

## Author: Harun Sasmaz

## ID: 59900

## Structures

> Plane, Queue

## Utility Functions

> enqueue(), dequeue(), top(), createQueue(), print_queue(),
> log(), log_header(), pthread_sleep()

## Important Global Variables

> Queue landing, departing, emergency
> pthread_mutex_t runway_lock, start_lock
> pthread_cond_t runway_cond
> Plane all_planes[]

## Main functions

> landing_func(), departing_func(), air_control()

## Implementation

All parts are working fine, however due to type conversion to integers, some floating points are lost. That results in 1 second runway time in some flights. That information is misleading but program does work properly. Also, in emergency flights sometimes it seems like it comes one second early, again it is due to floating points to integer conversion.

Landing function first creates a plane, and initializes it. We have a global plane array to be able reach their lock from ATC thread. Then it acquires the global mutex lock to be able to enqueue itself to the related queue, either landing or emergency. If plane's id is 1, i.e. the first plane, it sends a signal to ATC thread to start accepting planes. After that, it waits for its turn to be able to use the runway, this waiting is achieved by pthread_cond_timedwait and the cond variable inside plane. pthread_cond_timedwaid is used to terminate threads when simulation time is over. After waiting is done, the thread releases its lock and exit.

Departing function is also same as the landing function, it creates a plane, adds that plane to the global plane array, acquires the global lock to enqueue itself and wait for air traffic control thread to continue. It also uses timed wait to be able to exit from function. Only difference is this function uses departing queue to enqueue a plane.

Air traffic control function is working from the beginning of the program to the end. However, initially it used the global cond variable to wait for the first plane. After that, it dequeues an element from a queue and signal to that plane by using plane's id and its cond variable so that plane can continue to its operation. Also, ATC thread keeps the log of each plane about land/depart. Since any operation takes 2t seconds, after each signal, tower waits for 2t seconds by using pthread_sleep(). To decide which queue to pop an element, waiting time of departing planes and sizes of queues are in consideration. Of course emergency queue has the priority. After that, we check the sizes of landing queue and departing queue to avoid deadlocks. Since sometimes it is not enough, I also add a parameter waiting time which refers to the waiting time of the first plane in the departing queue, if it is higher than 10 seconds, the tower favors a departing plane. Once signalling is done, it releases the lock and sleeps for 2 seconds.

In the main function, we first parse command line arguments and set related variables such as simulation time, probability etc. I declare a pthread_t array to store each thread. Create the atc thread, start the simulation and add one landing, one departing plane. After that, in a while loop it creates related plane thread according to probability and current time. After simulation is done, we iterate through pthread_t array and join them to the master thred, destroy lock and cond variables, free the queues.
