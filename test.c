#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "fiber.h"

#define NUM_FIBER 11

fiber_t fibers[NUM_FIBER];
ucontext_t parent, child;
long long int somando = 0;
//int finish = 0;

void *threadFunction11(void * c) {
    printf("Thread 11 começou.\n\n");
	void * retval = (void *) malloc(sizeof(long long int));
	*(long long int *) retval = somando;
    while(*(long long int *) retval < 40000000)
        *(long long int *)retval = *(long long int *)retval + 1;
    printf("Thread 11 terminou. Seu valor de retorno é: %lld.\n\n", *(long long int *)retval);
    fiber_exit(retval);
}

void *threadFunction1(void * a) {
    printf("Thread 1 começou.\n");
	void ** joinRetval = (void**) malloc(sizeof(void *));
    printf("Thread 1 deu join na thread 10.\n\n");
    fiber_join(fibers[9], joinRetval);
    printf("Thread 1 retornou do join.\n");
	void * retval = (void *) malloc(sizeof(int));
	*(int *)retval = 10;
	printf("Thread 1 terminou. Retval da t1: %d. JoinRetval da t1: %d.\n\n", *(int *)retval, **(int **)joinRetval);
    fiber_exit(retval);
}

void *threadFunction2(void * b) {
    printf("Thread 2 começou e deu join na thread 5. Valor atual da variável global: %lld.\n\n", somando);
    fiber_join(fibers[4], NULL);
    printf("Thread 2 retornou do join.\n");
    printf("Thread 2 terminou.\n\n");
    fiber_exit(NULL);
}

void *threadFunction3(void * c) {
    printf("Thread 3 começou. Valor atual da variável global: %lld.\n\n", somando);
    while(somando < 4000000000)
        somando++;
    printf("Thread 3 terminou. Valor atual da variável global: %lld.\n\n", somando);
    fiber_exit(NULL);
}

void *threadFunction4(void * c) {
    printf("Thread 4 começou e terminará agora. valor atual da variável global: %lld.\n\n", somando);
    fiber_exit(NULL);
}

void *threadFunction5(void * c) {
    printf("Thread 5 começou. Valor atual da variável global: %lld.\n", somando);
    void ** joinretval = (void **) malloc(sizeof(void *));
    printf("Thread 5 deu join na thread 7.\n\n");
    fiber_join(fibers[6], joinretval);
    printf("Thread 5 retornou do join. O valor de retorno da thread 7 é: %lld\n\n", **(long long int**) joinretval);
    while(somando > 10000000)
        somando--;
    printf("Thread 5 terminou. Valor atual da variável global: %lld.\n\n", somando);
    fiber_exit(NULL);
}

void *threadFunction6(void * c) {
    printf("Thread 6 começou e deu join na thread 1. Valor atual da variável global: %lld.\n\n", somando);
    fiber_join(fibers[0], NULL);
    printf("Thread 6 retornou do join e terminou. Valor atual da variável global: %lld.\n\n", somando);
    fiber_exit(NULL);
}

void *threadFunction7(void * c) {
    printf("Thread 7 começou. Valor atual da variável global: %lld.\n", somando);
    void ** joinRetval = (void **) malloc(sizeof(void *));
    void * retval = (void *) malloc(sizeof(long long int));
    *(long long int *) retval = somando;
    while(*(long long int *) retval > 0  && *(long long int *) retval < 4000000)
        *(long long int *)retval = *(long long int *)retval + 1;
    fiber_create(&fibers[10], threadFunction11, NULL);
    printf("Thread 7 criou a thread 11 e deu join na thread 11.\n\n");
    fiber_join(fibers[10], joinRetval);
    printf("Thread 7 retornou do join e terminou. Seu valor de retorno é %lld e o valor de retorno da thread 11 é: %lld.\n\n", somando, **(long long int **) joinRetval);
    fiber_exit(retval);
}

void *threadFunction8(void * c) {
    printf("Thread 8 começou e deu join na thread 7.\n\n");
    fiber_join(fibers[6], NULL);
    printf("Thread 8 retornou do join. Thread 8 terminou.\n\n");
    fiber_exit(NULL);
}

void *threadFunction9(void * c) {
    printf("Thread 9 começou e terminará agora.\n\n");
    fiber_exit(NULL);
}

void *threadFunction10(void * c) {
    printf("Thread 10 começou.\n");
	void * retval = (void *) malloc(sizeof(int));
	*(int *) retval = 15;
    printf("Thread 10 terminou. Seu valor de retorno é 15.\n\n");
    fiber_exit(retval);
}

int main () {

    void * arg = NULL;
	void ** retval = (void **) malloc(sizeof(void *));
    printf("Thread principal começou.\n");
    
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
    printf("Thread principal criou 10 threads.\n");

	
    printf("Thread principal deu join na thread 1.\n\n");
    fiber_join(fibers[0], retval);
    printf("Thread principal retornou do join.\n");
	printf("O valor de retorno da thread 1, recebido pela thread principal, é %d.\n", **(int **)retval);
    printf("Thread principal terminou.\n\n");
	fiber_exit(NULL);

    return 0;
}