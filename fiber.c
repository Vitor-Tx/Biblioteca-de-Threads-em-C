#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <pthread.h>

/*  
    Implementação de threads em user-level(fibers) no modelo M para 1(M threads user-level 
    para 1 thread kernel-level) com escalonamento utilizando o algoritmo round-robin.

    by Guilherme Bartasson, Diego Batistuta e Vitor Teixeira, 2019
*/

typedef int fiber_t; // tipo para ID de fibers

/*
    Struct de uma fiber
*/
typedef struct {
    fiber_t * next;
    fiber_t * prev;
    fiber_t * fiber_list;
    fiber_t * this;
    fiber_t fiberId;
    fiber_t state;
}fiber;

/*
    Lista que armazenará as fibers criadas pela biblioteca
*/
typedef struct {
    fiber * threads;
}fiber_list;


/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine.

*/
int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg) {
    pthread_create(fiber, NULL, start_routine, arg);
}

/*
    fiber_join
    ----------

    Faz com que a thread atual espere o término de outra thread para começar a executar.

*/
int fiber_join(fiber_t fiber, void **retval);

/*
    fiber_destroy
    -------------

    Destrói uma fiber correspondente ao seu ID fiber.

*/
int fiber_destroy(fiber_t fiber);

/*
    fiber_exit
    ----------

    Para a execução da thread atual.

*/
void fiber_exit(void *retval);
