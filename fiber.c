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
typedef struct{
    struct fiber_struct * next;
    struct fiber_struct * prev;
    struct fiber_list * fiber_list;
    ucontext_t * context;
    fiber_t * fiberId;
}fiber_struct;


/*
    Struct da lista que armazenará as fibers criadas pela biblioteca
*/
typedef struct{
    struct fiber_struct * fibers;
}fiber_list;

// Lista global que armazenará as fibers
fiber_list * f_list = NULL;


int fiberSchedule(){

}

int initFibers(){

    if (f_list == NULL) {
        f_list = (fiber_list*) malloc(sizeof(fiber_list));
        f_list->fibers = NULL;
    }
    // Caso não haja nenhuma fiber na lista
    if (f_list->fibers == NULL) {
        // Variável utilizada para armazenar o contexto do processo pai
        ucontext_t parentContext;

        // Obtendo o contexto do processo pai
        getcontext(&parentContext);

        // Criando a fiber do processo pai
        fiber_struct * parentFiber = (fiber_struct *) malloc(sizeof(fiber_struct));

        // Inicializando a fiber do processo pai
        parentFiber->context = &parentContext;
        parentFiber->fiberId = NULL;
        parentFiber->prev = NULL;
        parentFiber->next = NULL; // Fiber recém criada
        parentFiber->fiber_list = (struct fiber_list *) f_list;

        // Adicionado a fiber do processo pai como o primeiro elemento da lista
        f_list->fibers = (struct fiber_struct *) parentFiber; 
    } 


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
    fiber_struct * fiber = (fiber_struct *) f_list->fibers;

    // Procurando a fiber que possui o id fornecido
    while (fiber != NULL && fiber->fiberId != fiberId) {
        fiber = (fiber_struct *) fiber->next;
    }

    // Caso a fiber não tenha sido econtrada
    if(fiber == NULL) return 2;

    // Setando o contexto da fiber encontrada
    setcontext(fiber->context);

    return 0;
}

/*
    Insere uma fiber na última posição da lista de fibers
*/
void pushFiber(fiber_struct * fiber) {
    // Inicializando a f_list caso ela não tenha sido inicializada
    if (f_list == NULL) {
        f_list = (fiber_list*) malloc(sizeof(fiber_list));
        f_list->fibers = NULL;
    }
    
    // Caso não haja nenhuma fiber na lista
    if (f_list->fibers == NULL) {
        // Variável utilizada para armazenar o contexto do processo pai
        ucontext_t parentContext;

        // Obtendo o contexto do processo pai
        getcontext(&parentContext);

        // Criando a fiber do processo pai
        fiber_struct * parentFiber = (fiber_struct *) malloc(sizeof(fiber_struct));

        // Inicializando a fiber do processo pai
        parentFiber->context = &parentContext;
        parentFiber->fiberId = NULL;
        parentFiber->prev = NULL;
        parentFiber->next = (struct fiber_struct *) fiber; // Fiber recém criada
        parentFiber->fiber_list = (struct fiber_list *) f_list;

        // Adicionado a fiber do processo pai como o primeiro elemento da lista
        f_list->fibers = (struct fiber_struct *) parentFiber; 

        // Inserindo a fiber como segundo elemento da lista
        fiber->prev = (struct fiber_struct *) parentFiber;
        fiber->next = NULL;
        fiber->fiber_list = (struct fiber_list *) f_list;
    } 
    else { // Caso haja uma ou mais fibers na lista
        fiber_struct * f_aux1 = (fiber_struct *) f_list->fibers;

        // Procurando o último elemento da lista de fibers
        while (f_aux1->next != NULL) {
            f_aux1 = (fiber_struct *) f_aux1->next;
        }

        // Inserindo a nova fiber no fim da lista
        f_aux1->next = (struct fiber_struct *) fiber;
        fiber->prev = (struct fiber_struct *) f_aux1;
        fiber->next = NULL;
        fiber->fiber_list = (struct fiber_list *) f_list;
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
    ucontext_t * fiber = (ucontext_t *) malloc(sizeof(ucontext_t));

    // Convertento a função passada para um tipo aceito
    // pela system_call makecontext()
    void (*routine_aux)(void) = (void (*)(void )) start_routine; 

    // Struct que irá armazenar a nova fiber
    fiber_struct * f_struct = (fiber_struct *) malloc(sizeof(fiber_struct));
    
    // Obtendo o contexto atual e armazenando-o na variável fiber
    getcontext(fiber);

    // Modificando o contexto para uma nova pilha
    fiber->uc_link = 0;
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
    f_struct->fiber_list = NULL;

    // Inserindo a nova fiber na lista de fibers
    pushFiber(f_struct);

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

*/
int fiber_destroy(fiber_t fiber);

/*
    fiber_exit
    ----------

    Para a execução da thread atual.
r
*/
void fiber_exit(void *retval){
    fiber_struct * aux = (fiber_struct *) f_list->fibers;

    setcontext(aux->context);
}
