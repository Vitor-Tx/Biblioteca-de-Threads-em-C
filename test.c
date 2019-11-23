#include <stdio.h>
#include <ucontext.h>
#include "fiber.h"

#define NUM_FIBER 2
fiber_t * fibers[NUM_FIBER];

void *threadFunction1(void * a) {
    printf("thread1\n");
    // swapcontext( &child, &parent );
    // printf( "Child thread exiting\n" );
    // swapcontext( &child, &parent );
}

void *threadFunction2(void * b) {
    printf( "thread2\n" );
    // swapcontext( &child, &parent );
    // printf( "Child thread exiting\n" );
    // swapcontext( &child, &parent );
}

int main () {
    void * arg = 0;
    
    fiber_create(fibers[0], threadFunction1, arg);
    fiber_create(fibers[1], threadFunction2, arg);

    fiberSwap(fibers[0]);

    return 0;
}