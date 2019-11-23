#include <stdio.h>
#include <ucontext.h>
#include "fiber.h"

#define NUM_FIBER 2

void *threadFunction1(void * a) {
    printf( "Child fiber yielding to parent" );
    // swapcontext( &child, &parent );
    // printf( "Child thread exiting\n" );
    // swapcontext( &child, &parent );
}

void *threadFunction2(void * b) {
    printf( "Child fiber yielding to parent" );
    // swapcontext( &child, &parent );
    // printf( "Child thread exiting\n" );
    // swapcontext( &child, &parent );
}

int main () {
    void * arg = 0;
    fiber_t fibers[NUM_FIBER] = {1, 2};
    
    fiber_create(&fibers[0], threadFunction1, 0);
    // fiber_create(&fibers[1], threadFunction2, arg);
}