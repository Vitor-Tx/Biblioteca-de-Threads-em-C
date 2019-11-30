/*  
    Implementação de threads em user-level(fibers) no modelo M para 1(M threads user-level 
    para 1 thread kernel-level) com escalonamento utilizando o algoritmo round-robin.

    by Guilherme Bartasson, Diego Batistuta e Vitor Teixeira, 2019
*/

#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include "fiber.h"

// Pilha de 64kB
#define FIBER_STACK 1024*64

typedef unsigned int fiber_t; // tipo para ID de fibers

/*
    Struct de uma fiber
*/
typedef struct{
    struct Fiber * next;
    struct Fiber * prev;
    ucontext_t * context;
    int status; // para fiberjoin, fiberexit e fiberdestroy
    fiber_t * fiberId;
}Fiber;

ucontext_t schedulerFiber, parentContext;

/*
    Struct da lista que armazenará as fibers criadas pela biblioteca
*/
typedef struct{
    struct Fiber * fibers;
    struct Fiber * currentFiber;
    int nFibers;
}FiberList;

// Lista global que armazenará as fibers
FiberList * f_list = NULL;

void timeHandler(){
    Fiber * currentFiber = (Fiber *) f_list->currentFiber;
    swapcontext(currentFiber->context, &schedulerFiber);
}

void getnFibers() {
    printf("%d\n", f_list->nFibers);
}

void fiberScheduler() {
    Fiber * currentFiber = (Fiber *) f_list->currentFiber;
    Fiber * proximaFiber = (Fiber *) currentFiber->next;

	f_list->currentFiber = (struct Fiber *) proximaFiber;
	setcontext(proximaFiber->context);
}

void startFibers() {
    struct sigaction sa;
    struct itimerval timer;
    int seconds = 0;
    int microSeconds = 500;

    Fiber * aux = (Fiber *) f_list->fibers;
    aux = (Fiber *) aux->next;
    f_list->currentFiber = (struct Fiber *) aux;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timeHandler;
    sigaction (SIGVTALRM, &sa, NULL);
 
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = microSeconds;
 
    timer.it_interval.tv_sec = seconds;
    timer.it_interval.tv_usec = microSeconds;

    setitimer (ITIMER_VIRTUAL, &timer, NULL);

    setcontext(aux->context);
}

/*
    fiberSwap
    ---------

    Função que troca o contexto da thread atual com o contexto da thread selecionada

    retorna 0 se der tudo certo, e retorna um inteiro maior que 0 caso ocorra algum erro.

*/
int fiberSwap(fiber_t * fiberId) {
    if(fiberId == NULL) return 1;
    
    // Obtendo a primeira fiber da lista de fibers
    Fiber * fiber = (Fiber *) f_list->fibers;

    // Procurando a fiber que possui o id fornecido
    while (fiber != NULL && fiber->fiberId != fiberId) {
        fiber = (Fiber *) fiber->next;
    }

    // Caso a fiber não tenha sido econtrada
    if(fiber == NULL) return 2;

    // Setando o contexto da fiber encontrada
    setcontext(fiber->context);

    return 0;
}

int initFiberList() {
    if (f_list == NULL) {
        f_list = (FiberList *) malloc(sizeof(FiberList));
        f_list->fibers = NULL;
        f_list->nFibers = 0;
        f_list->currentFiber = NULL;
    }
    // Caso não haja nenhuma fiber na lista
    if (f_list->fibers == NULL) {

        // Criando a fiber do processo pai
        Fiber * parentFiber = (Fiber *) malloc(sizeof(Fiber));

        // Inicializando a fiber do processo pai
        parentFiber->context = &parentContext;
        parentFiber->fiberId = NULL;
        parentFiber->prev = NULL;
        parentFiber->next = NULL; 

        // Adicionado a fiber do processo pai como o primeiro elemento da lista
        f_list->fibers = (struct Fiber *) parentFiber;

        f_list->nFibers++;

		getcontext(&schedulerFiber);
        
        schedulerFiber.uc_link = &parentContext;
        schedulerFiber.uc_stack.ss_sp = malloc(FIBER_STACK);
        schedulerFiber.uc_stack.ss_size = FIBER_STACK;
        schedulerFiber.uc_stack.ss_flags = 0;        
        if (schedulerFiber.uc_stack.ss_sp == 0 ) {
            printf("malloc: Não foi possível alocar a pilha para o escalonador");
            return 1;
        }

        makecontext(&schedulerFiber, fiberScheduler, 1, NULL);
    }
	return 0;
}

/*
    Insere uma fiber na última posição da lista de fibers
*/
void pushFiber(Fiber * fiber) {
    
    initFiberList();
    
    Fiber * f_aux1 = (Fiber *) f_list->fibers;

    f_aux1->prev = (struct Fiber *) fiber;

    // Procurando o último elemento da lista de fibers
    for (int x = 1; x < f_list->nFibers; x++) {
        f_aux1 = (Fiber *) f_aux1->next;
    }

    // Inserindo a nova fiber no fim da lista
    f_aux1->next = (struct Fiber *) fiber;
    fiber->prev = (struct Fiber *) f_aux1;
    fiber->next = f_list->fibers;

    f_list->nFibers++;
    
}

/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine.

*/
int fiber_create(fiber_t *fiberId, void *(*start_routine) (void *), void *arg) {
    // Variável que irá armazenar a nova fiber 
    ucontext_t * fiber = (ucontext_t *) malloc(sizeof(ucontext_t));

    // Struct que irá armazenar a nova fiber
    Fiber * f_struct = (Fiber *) malloc(sizeof(Fiber));
    
    // Obtendo o contexto atual e armazenando-o na variável fiber
    getcontext(fiber);

    // Modificando o contexto para uma nova pilha
    fiber->uc_link = &schedulerFiber;
    fiber->uc_stack.ss_sp = malloc(FIBER_STACK);
    fiber->uc_stack.ss_size = FIBER_STACK;
    fiber->uc_stack.ss_flags = 0;        
    if (fiber->uc_stack.ss_sp == 0 ) {
        printf("malloc: Não foi possível alocar a pilha");
        return 1;
    }

    // Criando a fiber propriamente dita
    makecontext(fiber, (void (*)(void )) start_routine, 1, arg);

    // Inicializando a struct que armazena a fiber recem criada
    f_struct->context = fiber;
    f_struct->fiberId = fiberId;
    f_struct->prev = NULL;
    f_struct->next = NULL;

    // Inserindo a nova fiber na lista de fibers
    pushFiber(f_struct);
    
    //pegando o contexto da thread principal e passando para o parentcontext(assim
    // o atualizando sempre que a fiber_create for chamada)
    Fiber * parent = (Fiber *) f_list->fibers;
    getcontext(parent->context);
    return 0;
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
    (INCOMPLETA!!! NÃO DEVE SER USADA, POIS NÃO ESTÁ 
    SETANDO O NOVO CONTEXT NEM VERIFICANDO SE A FIBER 
    ATUAL ESTÁ RODANDO!)

*/
int fiber_destroy(fiber_t fiberId){
    if(fiberId < 0) 
        return 1;
    
    // Obtendo a primeira fiber da lista de fibers
    Fiber * fiber = (Fiber *) f_list->fibers;

    // Procurando a fiber que possui o id fornecido
    int i = 1;
    while (i < f_list->nFibers && *fiber->fiberId != fiberId) {
        fiber = (Fiber *) fiber->next;
        i++;
    }

    // Caso a fiber não tenha sido econtrada
    if(i==f_list->nFibers) 
        return 3;

    //pegando a fiber anterior a fiber que vai ser destroida
    Fiber * f_ant = (Fiber *) fiber->prev;

    //pegando a proxima fiber da fiber que vai ser destroida
    Fiber * f_prox = (Fiber *) fiber->next;

    // tirando a fiber que ira ser destroida da lista circular
    f_ant->next = (struct Fiber *) f_prox;
    f_prox->prev = (struct Fiber *) f_ant;

    //destruindo a fiber
    free(fiber->context);
    free(fiber);

    return 0;

}

/*
    fiber_exit
    ----------

    Para a execução da thread atual.

*/
void fiber_exit(void *retval){

    Fiber * currentFiber = (Fiber *) f_list->currentFiber;
    if(currentFiber->fiberId != NULL){

    }
    Fiber * aux = (Fiber *) f_list->fibers;

    setcontext(aux->context);
}
