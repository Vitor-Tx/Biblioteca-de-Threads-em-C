#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "fiber.h"

#define NUM_FIBER 20

fiber_t fibers[NUM_FIBER];
ucontext_t parent, child;
long long int somando = 0;
//int finish = 0;

void *threadFunction11(void * c) {
    printf("Thread 11\n");
	void * retval = (void *) malloc(sizeof(long long int));
	*(long long int *) retval = somando;
    while(*(long long int *) retval > 0  && *(long long int *) retval < 4000000)
        *(long long int *)retval = *(long long int *)retval + 1;
    printf("Thread 11 saiu do loop\n");
    fiber_exit(retval);
}

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
    fiber_join(fibers[4], NULL);
    printf("Thread 2 esperou a thread 5\n");
    fiber_exit(NULL);
}

void *threadFunction3(void * c) {
    printf("Thread 3\n");
    while(somando < 4000000000)
        somando++;
    fiber_exit(NULL);
}

void *threadFunction4(void * c) {
    printf("Thread 4. valor do inteiro atual: %lld\n", somando);
    fiber_exit(NULL);
}

void *threadFunction5(void * c) {
    printf("Thread 5.\n");
    void ** joinretval = (void **) malloc(sizeof(void *));
    fiber_join(fibers[6], joinretval);
    printf("Thread 5. valor do retval da thread 7 atual: %lld\n", **(long long int**) joinretval);
    while(somando > -40000)
        somando--;
    printf("Thread 5 saiu do loop\n");
    fiber_exit(NULL);
}

void *threadFunction6(void * c) {
    printf("Thread 6.\n");
    fiber_join(fibers[0], NULL);
    printf("Thread 6. valor do inteiro atual: %lld\n", somando);
    fiber_exit(NULL);
}

void *threadFunction7(void * c) {
    printf("Thread 7\n");
    void ** joinRetval = (void **) malloc(sizeof(void *));
    void * retval = (void *) malloc(sizeof(long long int));
    *(long long int *) retval = somando;
    while(*(long long int *) retval > 0  && *(long long int *) retval < 4000000)
        *(long long int *)retval = *(long long int *)retval + 1;
    fiber_create(&fibers[10], threadFunction11, NULL);
    printf("Thread 7 criou a thread 11\n");
    fiber_join(fibers[10], joinRetval);
    printf("Thread 7 esperou a thread 11. Valor do retval ta thread 11: %lld\n", **(long long int **) joinRetval);

    fiber_exit(retval);
}

void *threadFunction8(void * c) {
    printf("Thread 8\n");
    fiber_join(fibers[6], NULL);
    printf("Thread 8 esperou a thread 7 terminar\n");
    fiber_exit(NULL);
}

void *threadFunction9(void * c) {
    printf("Thread 9\n");
    fiber_exit(NULL);
}

void *threadFunction10(void * c) {
    printf("Thread 10\n");
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