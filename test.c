#include <stdio.h>
#include <ucontext.h>
#include "fiber.h"

#define NUM_FIBER 4

fiber_t fibers[NUM_FIBER];
ucontext_t parent, child;
int finish = 0;

void *threadFunction1(void * a) {
    printf("thread1\n");

    fiberSwap(&fibers[1]);
}

void *threadFunction2(void * b) {
    printf( "thread2\n" );

    fiberSwap(&fibers[2]);
}

void *threadFunction3(void * c) {
    printf( "thread3\n" );

    fiberSwap(&fibers[3]);
}

void *threadFunction4(void * c) {
    printf( "thread4\n" );

    finish = 1;

    setcontext(&parent);
}

int main () {
    getcontext(&parent);

    if (finish == 0) {
        void * arg = NULL;

        fiber_create(&fibers[0], threadFunction1, arg);
        fiber_create(&fibers[1], threadFunction2, arg);
        fiber_create(&fibers[2], threadFunction3, arg);
        fiber_create(&fibers[3], threadFunction4, arg);

        fiberSwap(&fibers[0]);
    } else {
        printf("Processo completo\n");
    }

    return 0;
}