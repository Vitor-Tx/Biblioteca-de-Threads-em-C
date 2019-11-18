#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>

/*  
    Implementação de threads em user-level(fibers) no modelo M para 1(M threads user-level 
    para 1 thread kernel-level) com escalonamento utilizando o algoritmo round-robin.

    by Guilherme Bartasson, Diego Batistuta e Vitor Teixeira, 2019
*/

// Pilha de 64kB
#define FIBER_STACK 1024*64

typedef int fiber_t; // tipo para ID de fibers

/*
    Struct de uma fiber
*/
typedef struct {
    fiber_t * next;
    fiber_t * prev;
    fiber_t * fiber_list;
    ucontext_t context;
    fiber_t fiberId;
}fiber_struct;

/*
    Struct da lista que armazenará as fibers criadas pela biblioteca
*/
typedef struct {
    fiber_struct * fibers;
}fiber_list;

// Lista global que armazenará as fibers
fiber_list f_list;

/*
    Insere uma fiber na última posição da lista de fibers
*/
void pushFiber(fiber_struct * fiber) {
    // Caso não haja nenhuma fiber na lista
    if (f_list.fibers = NULL) {
        f_list.fibers = fiber;

        fiber->prev = NULL;
        fiber->next = NULL;
        fiber->fiber_list = &f_list;
    } else { // Caso contrário
        fiber_struct * f_aux1 = f_list->fibers;
        fiber_t * f_aux2 = f_aux1->next;

        // Procurando o último elemento da lista de fibers
        while (f_aux2 != NULL) {
            f_aux2 = f_aux2->next;
        }

        // Inserindo a nova fiber no fim da lista
        f_aux2->next = fiber;
        fiber->prev = f_aux2;
        fiber->next = NULL;
        fiber->fiber_list = &f_list;
    }
}

/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine.

*/
int fiber_create(fiber_t *fiberId, void *(*start_routine) (void *), void *arg) {

    // Variável que irá armazenar a nova fiber 
    ucontext_t fiber;

    // Struct que irá armazenar a nova fiber
    fiber_struct * f_struct;

    // Obtendo o contexto atual e armazenando-o na variável fiber
    getcontext(&fiber);

    // Modificando o contexto para um nova pilha
    fiber.uc_link = 0;
    fiber.uc_stack.ss_sp = malloc( FIBER_STACK );
    fiber.uc_stack.ss_size = FIBER_STACK;
    fiber.uc_stack.ss_flags = 0;        
    if (fiber.uc_stack.ss_sp == 0 ) {
        perror("malloc: Não foi possível alocar a pilha");
        exit(1);
    }

    // Criando a fiber propriamente dita
    makecontext(&fiber, start_routine, arg);

    // Inicializando a struct que armazena a fiber recem criada
    f_struct->context = &fiber;
    f_struct->fiberId = fiberId;
    f_struct->prev = NULL;
    f_struct->next = NULL;
    f_struct->fiber_list = NULL;

    // Inserindo a nova fiber na lista de fibers
    pushFiber(f_struct);
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
