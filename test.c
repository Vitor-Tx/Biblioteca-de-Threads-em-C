#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "fiber.h"

#define NUM_FIBER 10

fiber_t fibers[NUM_FIBER];
ucontext_t parent, child;
//int finish = 0;

void *threadFunction1(void * a) {
    printf("Thread 1\n");
	void ** joinRetval = (void**) malloc(sizeof(void *));
    fiber_join(fibers[9], joinRetval);
    printf("Thread 1 esperou a thread 10\n");
	void * retval = (void *) malloc(sizeof(int));
	*(int *)retval = 10;
	printf("retval da tf1: %d. JoinRetval da tf1: %d.\n", *(int *)retval, **(int **)joinRetval);
    fiber_exit(retval);
}

void *threadFunction2(void * b) {
    printf("Thread 2\n");
    fiber_exit(NULL);
}

void *threadFunction3(void * c) {
    printf("Thread 3\n");
    fiber_exit(NULL);
}

void *threadFunction4(void * c) {
    printf("Thread 4\n");
    fiber_exit(NULL);
}

void *threadFunction5(void * c) {
    printf("Thread 5\n");
    fiber_exit(NULL);
}

void *threadFunction6(void * c) {
    printf("Thread 6\n");
    fiber_exit(NULL);
}

void *threadFunction7(void * c) {
    printf("Thread 7\n");
    fiber_exit(NULL);
}

void *threadFunction8(void * c) {
    printf("Thread 8\n");
    fiber_exit(NULL);
}

void *threadFunction9(void * c) {
    printf("Thread 9\n");
    fiber_exit(NULL);
}

void *threadFunction10(void * c) {
    printf("Thread 10\n\n");
	void * retval = (void *) malloc(sizeof(int));
	*(int *) retval = 15;
    fiber_exit(retval);
}


int main () {

    void * arg = NULL;
	void ** retval = (void **) malloc(sizeof(void *));

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
	
    
    fiber_join(fibers[0], retval);
	printf("retval na main: %p, %p, %d\n", *retval, retval, **(int **)retval);
    printf("Thread principal esperou a thread 1(que tinha esperado a thread 10). Processo completo\n");
	fiber_exit(NULL);

    return 0;
}