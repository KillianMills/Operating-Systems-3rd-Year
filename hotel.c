//Killian Mills 11368701
//Mark McCluskey 12514857
 
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
 
#define ETIMEDOUT 110
#define HEAP_INITIAL_CAPACITY 1
 
typedef struct{
        int expired; // number of wakeup calls that have taken place
 
        int size;       // elements currently in array
        int capacity;   // limit of array or total size
        long *data;     // pointer to array of longs
        int *room; // pointer to array of ints
 
        pthread_mutex_t mutex;
        pthread_cond_t cond;
 
} Heap;
 
// constructor for our heap
void heapInit(Heap *heap){
 
        heap->expired = 0;
        heap->size = 0;
        heap->capacity = HEAP_INITIAL_CAPACITY; // default heap size is 1
        heap->data = malloc(sizeof(int) * heap->capacity);
        heap->room = malloc(sizeof(int) * heap->capacity);
}
 
 
// doubles the size of the underlying data array capactiy if size >= capacity
void heapDoubleSize(Heap *heap){
 
        if(heap->size >= heap->capacity-1){
                heap->capacity *= 2;
                heap->data = realloc(heap->data, sizeof(long) * heap->capacity);
                heap->room = realloc(heap->room, sizeof(int) * heap->capacity);
        }
}
 
// restructure the tree as we add an element
void fixUpHeap(Heap *heap, int position){
 
        int parent, temp,temp2;
        if(position != 0){
                parent = (position-1) / 2;     
 
                if(heap->data[parent] > heap->data[position]){
                        //swap
                        temp = heap->data[parent];
                        temp2 = heap->room[parent];
 
                        heap->data[parent] = heap->data[position];
                        heap->room[parent] = heap->room[position];
 
                        heap->data[position] = temp;
                        heap->room[position] = temp2;
 
                        fixUpHeap( heap, parent);
                }
        }
}
 
// inserts an element to the tree, calls fixUpHeap
void insertHeap(Heap *heap,int roomN, long newAlarm){
 
        heapDoubleSize(heap);
        heap->size++;
        heap->data[heap->size-1] = newAlarm; // new alarm time
        heap->room[heap->size-1] = roomN; // new room number
        fixUpHeap(heap, heap->size-1);
}
 
// restructure the tree as we remove an element
void fixDownHeap(Heap *heap, int position){
       
        int left, right, min, temp,temp2;
        left = 2* position +1;
        right = 2* position +2;
       
        if(right>= heap->size){
                if(left >= heap->size)
                        return;
                else
                        min = left;
        } else{
                if(heap->data[left] <= heap->data[right])
                        min= left;
                else
                        min = right;
        }
        if(heap->data[position] > heap->data[min]){
                //swap
                temp = heap->data[min];
                temp2= heap->room[min];
 
                heap->data[min] = heap->data[position];
                heap->room[min] = heap->room[position];
 
                heap->data[position] = temp;
                heap->room[position] = temp2;
 
                fixDownHeap(heap, min);
        }
}
 
// removes an element from the tree, calls fixDownHeap
void removeHeap(Heap *heap){
 
        if(heap->size!=0){
                heap->data[0] = heap->data[heap->size-1];
                heap->size--;
                if(heap->size > 0){
                        fixDownHeap(heap, 0);
                }
        }
}
 
 
// frees the memory allocated to the data array
void heapFree(Heap *heap){
       
        free(heap->data);
        free(heap->room);
        heap->size = 0;
        heap->capacity = 0;
        printf("Pending alarms: %d\n", heap->size);
}
//-----------END OF HEAP--------------
 
 
// used to register random future alarms for hotel guests
static void * generator(void *shared_in){
 
        //Bring struct into use
        Heap *heapShared = (Heap *)shared_in;
 
        int roomNumber;
 
        while(1){
                pthread_mutex_lock(&heapShared->mutex);
       
                srand (rand()%10000); //randomise
                roomNumber = 1 + (rand() % 5000);
                long alarmTime = time(NULL) + (rand() % 100);
 
                //add value to heap
                printf("Registering:\t%d %s\n", roomNumber, ctime(&alarmTime));
                insertHeap(heapShared,roomNumber,alarmTime);
 
                // if the first value is what we just submitted, alert the waker
                if(alarmTime==heapShared->data[0]){
                        pthread_cond_signal(&heapShared->cond);
 
                }
 
                pthread_mutex_unlock(&heapShared->mutex);
 
                int randomSleep = rand() % 5000000;
    usleep( randomSleep );
 
        }
       
        return ((void *)NULL);
}
 
 
// used to activate the alarms, wakes the hotel guests up
static void * waker(void *shared_in){
 
        //Bring struct into use
        Heap *heapShared = (Heap *)shared_in;
        int randomValue;
        int expired = 0;
        int removeTime;
        struct timespec timer;
 
        while(1){
 
                pthread_mutex_lock(&heapShared->mutex);
 
                //while no data to remove, wait
                while(&heapShared->data[0] == NULL){
                        pthread_cond_wait(&heapShared->cond, &heapShared->mutex);
 
                }
 
                //sets the seconds equal to the array[0]
                timer.tv_sec = heapShared->data[0];    
                timer.tv_nsec = 0;     
               
                removeTime = pthread_cond_timedwait(&heapShared->cond, &heapShared->mutex, &timer);
 
                //if the value at [0] is the current time
                if(removeTime == ETIMEDOUT){
 
                        printf("Removing:\t%d %s\n",heapShared->room[0], ctime(&heapShared->data[0]));
                        removeHeap(heapShared);
                        heapShared->expired++;
                        printf("Pending Alarms: %d\n", heapShared->size);
                        printf("Expired Alarms: %d\n\n", heapShared->expired);
 
                }
 
                pthread_mutex_unlock(&heapShared->mutex);
 
        }
 
        return ((void *)NULL);
}
 
int main(){
        //Make generator and waker threads
        pthread_t geneThread, wakeThread;
 
        //Start our heap
        Heap mainHeap;
        heapInit(&mainHeap);
 
        pthread_mutex_init(&mainHeap.mutex, NULL);
        pthread_cond_init(&mainHeap.cond, NULL);
 
        //Create our threads
        pthread_create(&geneThread, NULL, generator, (void *)&mainHeap);
        pthread_create(&wakeThread, NULL, waker, (void *)&mainHeap);
 
        //Signal blocker
        int sigHold; // hold the value of the current signal
        sigset_t mainBlock;
        sigemptyset(&mainBlock);
        sigaddset(&mainBlock, SIGINT);
        pthread_sigmask(SIG_BLOCK, &mainBlock, NULL); // block SIGINT
        sigwait(&mainBlock,&sigHold);   // wait for SIGINT
 
        //Cancel our threads
        pthread_cancel(geneThread);
        pthread_cancel(wakeThread);
 
        printf("\n");
 
        //Wait for our threads
        printf("The wake thread is cleaning up...\n");
        pthread_join(geneThread, NULL);
        printf("Goodbye from gene thread\n");
 
        printf("The gene thread is cleaning up...\n");
        pthread_join(wakeThread, NULL);
        printf("Goodbye from wake thread\n");
 
        pthread_mutex_destroy(&mainHeap.mutex);
        heapFree(&mainHeap);
 
  return (0);
}