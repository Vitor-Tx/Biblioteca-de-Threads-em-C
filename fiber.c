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
    status: 
    -  1: ativa
    -  0: esperando outra fiber com join
    - -1: terminada
*/
typedef struct Fiber{
    struct Fiber * next;        // Próxima fiber da lista
    struct Fiber * prev;        // Fiber anterior
    ucontext_t * context;       // Contexto da fiber
    int status;                 // para fiberjoin, fiberexit e fiberdestroy
    fiber_t fiberId;            // Id da fiber
    struct Fiber * joinFiber;
}Fiber;

/*
    Struct da lista que armazenará as fibers criadas pela biblioteca
*/
typedef struct FiberList{
    Fiber * fibers;             // Lista de fibers
    Fiber * currentFiber;       // FIber sendo executada no momento
    int nFibers;                // Quantidade de fibers na lista
}FiberList;

// Contexto do escalonador e do processo pai
ucontext_t schedulerContext, parentContext;

// Lista global que armazenará as fibers
FiberList * f_list = NULL;

/*
    Tratador do sinal do timer
*/
void timeHandler(){
    Fiber * currentFiber = (Fiber *) f_list->currentFiber;
    swapcontext(currentFiber->context, &schedulerContext);
}

/*
    Retorna a quantidade de fibers presentes na lista
*/
void getnFibers() {
    printf("%d\n", f_list->nFibers);
}

/*
    Escalonador de fibers
*/
void fiberScheduler() {
    Fiber * currentFiber = (Fiber *) f_list->currentFiber;
    Fiber * nextFiber = (Fiber *) currentFiber->next;

    while(nextFiber->status != 1) { // pulando a thread atual do loop não estiver pronta pra executar

        // Caso a fiber já tenha terminado
        if(nextFiber->status == -1) nextFiber = (Fiber *) nextFiber->next;
        
        // Caso a thread atual estiver num join
        if(nextFiber->status == 0) { 
            Fiber * joinFiber = (Fiber *) nextFiber->joinFiber;

            // Caso a thread que ela está esperando não tiver terminado
            if(joinFiber->status != -1) {
                nextFiber = (Fiber *) nextFiber->next; // pula a thread que está esperando
            } else {

                // Caso a thread que ela está esperando tenha terminado
                nextFiber->status = 1;
            }
        }
    } 
        
    // Definindo a próxima fiber selecionada como a fiber atual
	f_list->currentFiber = (Fiber *) nextFiber;

    // Setando o contexto da próxima fiber
	setcontext(nextFiber->context); 
}

/*
    Funçaõ responsável por iniciar o escalonado
*/
void startFibers() {
    struct sigaction sa;
    struct itimerval timer;
    int seconds = 0;
    int microSeconds = 500;

    // Obtendo o contexto da primeira fiber (que não é o processo pai)
    Fiber * aux = (Fiber *) f_list->fibers;
    aux = (Fiber *) aux->next;

    // Definindo a primeira fiber como a fiber atual
    f_list->currentFiber = (Fiber *) aux;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timeHandler;
    sigaction (SIGVTALRM, &sa, NULL);
 
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = microSeconds;
 
    timer.it_interval.tv_sec = seconds;
    timer.it_interval.tv_usec = microSeconds;

    setitimer (ITIMER_VIRTUAL, &timer, NULL);

    // Setando o contexto da primeira fiber
    setcontext(aux->context);
}

/*
    fiberSwap
    ---------

    Função que troca o contexto da thread atual com o contexto da thread selecionada

    retorna 0 se der tudo certo, e retorna um inteiro maior que 0 caso ocorra algum erro.

*/
int fiberSwap(fiber_t fiberId) {
    if (fiberId == 0) return 1;
    
    // Obtendo a primeira fiber da lista de fibers
    Fiber * fiber = (Fiber *) f_list->fibers;

    // Procurando a fiber que possui o id fornecido
    int i = 1;
    while (i < f_list->nFibers && fiber->fiberId != fiberId) {
        fiber = (Fiber *) fiber->next;
        i++;
    }

    // Caso a fiber não tenha sido econtrada
    if (i==f_list->nFibers) {
        return 3;
    }

    // Setando o contexto da fiber encontrada
    setcontext(fiber->context);

    return 0;
}

/*
    Função responsável por inicializar a lista de fibers caso ela não tenha
    sido inicializada ainda
*/
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
        parentFiber->fiberId = 0;
        parentFiber->prev = NULL;
        parentFiber->next = NULL; 
        parentFiber->status = 1;

        // Adicionado a fiber do processo pai como o primeiro elemento da lista
        f_list->fibers = (Fiber *) parentFiber;

        // Incrementando o número de fiber
        f_list->nFibers++;

        // Definindo o processo pai como fiber atual
        f_list->currentFiber = (Fiber *) parentFiber;

        // Obtendo um contexto para o escalonador
		getcontext(&schedulerContext);
        
        // Limpamdo o contexto recebido para o escalonador
        schedulerContext.uc_link = &parentContext;
        schedulerContext.uc_stack.ss_sp = malloc(FIBER_STACK);
        schedulerContext.uc_stack.ss_size = FIBER_STACK;
        schedulerContext.uc_stack.ss_flags = 0;        
        if (schedulerContext.uc_stack.ss_sp == 0 ) {
            printf("malloc: Não foi possível alocar a pilha para o escalonador");
            return 1;
        }

        // Criando o contexto do escalonador própriamente dito
        makecontext(&schedulerContext, fiberScheduler, 1, NULL);
    }
	return 0;
}

/*
    Insere uma fiber na última posição da lista de fibers
*/
void pushFiber(Fiber * fiber) {
    
    // Iniciando a lista de fibers
    initFiberList();
    
    // Obtendo a primeira fiber da lista (Processo pai)
    Fiber * firstFiber = (Fiber *) f_list->fibers;

    // Caso só haja a fiber do processo pai na lista
    if (f_list->nFibers == 1) {
        firstFiber->next = (Fiber *) fiber;
        firstFiber->prev = (Fiber *) fiber;
        fiber->prev = (Fiber *) firstFiber;
        fiber->next = f_list->fibers; 
    } else {
        // Caso haja outras fibers na lista

        Fiber * lastFiber = (Fiber *) firstFiber->prev;
        lastFiber->next = (Fiber *) fiber;
        fiber->next = (Fiber *) firstFiber;
        firstFiber->prev = (Fiber *) fiber;
    }

    // Incrementando o número de fibers
    f_list->nFibers++;
}

/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine.

*/
int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg) {
    // Variável que irá armazenar a nova fiber 
    ucontext_t * fiberContext = (ucontext_t *) malloc(sizeof(ucontext_t));

    // Struct que irá armazenar a nova fiber
    Fiber * fiberS = (Fiber *) malloc(sizeof(Fiber));
    
    // Obtendo o contexto atual e armazenando-o na variável fiberContext
    getcontext(fiberContext);

    // Modificando o contexto para uma nova pilha
    fiberContext->uc_link = &schedulerContext;
    fiberContext->uc_stack.ss_sp = malloc(FIBER_STACK);
    fiberContext->uc_stack.ss_size = FIBER_STACK;
    fiberContext->uc_stack.ss_flags = 0;        
    if (fiberContext->uc_stack.ss_sp == 0 ) {
        printf("malloc: Não foi possível alocar a pilha");
        return 1;
    }

    // Criando a fiber propriamente dita
    makecontext(fiberContext, (void (*)(void )) start_routine, 1, arg);

    // Inicializando a struct que armazena a fiber recem criada
    fiberS->context = fiberContext;
    fiberS->prev = NULL;
    fiberS->next = NULL;
    fiberS->status = 1;
    fiberS->joinFiber = NULL;


    // Inserindo a nova fiber na lista de fibers
    pushFiber(fiberS);

    // Atribuindo o id da fiber adequadamente
    * fiber = f_list->nFibers - 1;
    fiberS->fiberId = * fiber;

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
int fiber_join(fiber_t fiber, void **retval){
    int i;
    Fiber * currentFiber = (Fiber *) f_list->currentFiber;
    Fiber * fiberS = (Fiber *) f_list->fibers;

    // Encontrando a fiber com o id recebido
    for (i = 0; i < f_list->nFibers; i++) {
        fiberS = (Fiber *) fiberS->next;
        if(fiberS->fiberId == fiber)
            break;
    }

    // Se não for encontrado, a função retorna -1.
    if (i == f_list->nFibers) {
        printf("Erro -1 no join, colega\n");
        return -1;
    } 

    // Se a fiber que deveria terminar antes já terminou, retornar normalmente
    if(fiberS->status == -1) {
        return 0;
    }

    // Marcando a fiber atual como esperando
    currentFiber->status = 0;

    // Definindo a fiber que a fiber atual está esperando
    currentFiber->joinFiber = (Fiber *) fiberS;

    // Trocando para o contexto do escalonador
    swapcontext(currentFiber->context, &schedulerContext);

    // Exibido quando a fiber que realizou o join volta a executar
    printf("join deu certo!!!! Id da thread que retornou: %d\n", currentFiber->fiberId);

    // Definindo o status da fiber atual como pronta para executar
    currentFiber->status = 1;
    return 0;
}

/*
    fiber_destroy
    -------------

    Destrói uma fiber correspondente ao seu ID fiber.
    (não está pronta, não usem)

*/
int fiber_destroy(fiber_t fiber){
    if(fiber == 0){
        return 1;
    } 
        
    int i;

    // Obtendo a primeira fiber da lista de fibers e a fiber atual
    Fiber * fiberS = (Fiber *) f_list->fibers;
    Fiber * currentFiber = (Fiber *) f_list->currentFiber;

    if(currentFiber->fiberId == fiber){ //se a fiber atual tentar se destruir
        printf("Não é permitido que uma fiber destrua a si mesma");
        return 2;
    }

    for(i = 0; i < f_list->nFibers; i++){ // encontrando a fiber com o id recebido
        fiberS = (Fiber *) fiberS->next;
        if(fiberS->fiberId == fiber)
            break;
    }

    // Caso a fiber não tenha sido encontrada
    if(i == f_list->nFibers) 
        return 3;
    
    if(fiberS->status != -1) //por enquanto apenas fibers terminadas podem ser destruídas
        return 0;
    
    //pegando a fiber anterior a fiber que vai ser destruida
    Fiber * prevFiber = (Fiber *) fiberS->prev;

    //pegando a proxima fiber da fiber que vai ser destroida
    Fiber * nextFiber = (Fiber *) fiberS->next;

    // tirando a fiber que ira ser destroida da lista circular
    prevFiber->next = (Fiber *) nextFiber;
    nextFiber->prev = (Fiber *) prevFiber;

    //destruindo a fiber
    free(fiberS->context->uc_stack.ss_sp);
    free(fiberS->context);
    free(fiberS);

    f_list->nFibers--;
    printf("Fiber destruída com sucesso\n");

    return 0;
}

/*
    fiber_exit
    ----------

    Para a execução da thread atual.

*/
void fiber_exit(void *retval){

    Fiber * currentFiber = (Fiber *) f_list->currentFiber;

    // Caso a fiber atual seja o processo pai
    if (currentFiber->fiberId == 0){
        printf("Uso inadequado de fiber_exit(). Caso queira terminar o programa, use exit() em vez disso.\n");
        return;
    }

    // Definindo status da fiber atual como terminada
    currentFiber->status = -1;

    // Setando o contexto do escalonador
    setcontext(&schedulerContext);
}
