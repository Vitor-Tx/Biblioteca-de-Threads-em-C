#include <stdio.h>
#include <ucontext.h>
#include "fiber.h"

#define NUM_FIBER 10

fiber_t fibers[NUM_FIBER];
ucontext_t parent, child;
//int finish = 0;

void *threadFunction1(void * a) {
    printf("Thread 1\n");
    while(1);
}

void *threadFunction2(void * b) {
    printf("Thread 2\n");
    while(1);
}

void *threadFunction3(void * c) {
    printf("Thread 3\n");
    while(1);
}

void *threadFunction4(void * c) {
    printf("Thread 4\n");
    while(1);
}

void *threadFunction5(void * c) {
    printf("Thread 5\n");
    while(1);
}

void *threadFunction6(void * c) {
    printf("Thread 6\n");
    while(1);
}

void *threadFunction7(void * c) {
    printf("Thread 7\n");
    while(1);
}

void *threadFunction8(void * c) {
    printf("Thread 8\n");
    while(1);
}

void *threadFunction9(void * c) {
    printf("Thread 9\n");
    while(1);
}

void *threadFunction10(void * c) {
    printf("Thread 10\n\n");
    while(1);
}


int main () {
    //getcontext(&parent);

   // if (finish == 0) {
        void * arg = NULL;

        fiber_create(&fibers[0], threadFunction1, arg);
        fiber_create(&fibers[1], threadFunction2, arg);
        fiber_create(&fibers[2], threadFunction3, arg);
        fiber_create(&fibers[3], threadFunction4, arg);
        fiber_create(&fibers[4], threadFunction5, arg);
        fiber_create(&fibers[5], threadFunction6, arg);
        fiber_create(&fibers[6], threadFunction7, arg);
        fiber_create(&fibers[7], threadFunction8, arg);
        fiber_create(&fibers[8], threadFunction9, arg);
        fiber_create(&fibers[9], threadFunction10, arg);
        
        

        startFibers();
    //} else {
       //getnFibers();
        //printf("Processo completo\n");
    //}

    return 0;
}