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

## Tested command for the results

> ./out -p 0.5 -s 100 -seed 12345 -n 10

## Implementation

All parts are working fine, However, the global Plane array all_planes is initialized with a constant size, MAX_PLANES macro variable. If you set the simulation time more than 250, please update that variable to avoid any segmentation fault.

Landing function first creates a plane, and initializes it. We have a global plane array to be able reach their lock from ATC thread. Then it acquires the global mutex lock to be able to enqueue itself to the related queue, either landing or emergency. If plane's id is 1, i.e. the first plane, it sends a signal to ATC thread to start accepting planes. After that, it waits for its turn to be able to use the runway, this waiting is achieved by pthread_cond_timedwait and the cond variable inside plane. pthread_cond_timedwaid is used to terminate threads when simulation time is over. After waiting is done, the thread releases its lock and exit.

Departing function is also same as the landing function, it creates a plane, adds that plane to the global plane array, acquires the global lock to enqueue itself and wait for air traffic control thread to continue. It also uses timed wait to be able to exit from function. Only difference is this function uses departing queue to enqueue a plane.

Air traffic control function is working from the beginning of the program to the end. However, initially it used the global cond variable to wait for the first plane. After that, it dequeues an element from a queue and signal to that plane by using plane's id and its cond variable so that plane can continue to its operation. Also, ATC thread keeps the log of each plane about land/depart. Since any operation takes 2t seconds, after each signal, tower waits for 2t seconds by using pthread_sleep(). To decide which queue to pop an element, sizes of queues are in consideration. Of course emergency queue has the priority. After that, we check the sizes of landing queue and departing queue to avoid deadlocks. To be fair enough, if the size of landing queue is bigger than departing queue, then dequeue landing, and alsosymmetrically apply this to departing queue.

In the main function, we first parse command line arguments and set related variables such as simulation time, probability etc. I declare a pthread_t array to store each thread. Create the atc thread, start the simulation and add one landing, one departing plane. After that, in a while loop it creates related plane thread according to probability and current time. Also, after specified time, queues are printed out on console. After simulation is done, we iterate through pthread_t array and join them to the master thred, destroy lock and cond variables, free the queues.
