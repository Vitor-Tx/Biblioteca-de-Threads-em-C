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

typedef struct Waiting{
    fiber_t waitingId;
    struct Waiting * next;
}Waiting;

/*
    Struct de uma fiber.
    Atributos:

    - next e prev: ponteiros para outras estruturas
      de fibers para que seja feita uma lista circular.

    - context: ponteiro para a estrutura ucontext_t
      que contém o contexto da fiber.

    - status: representa o estado atual da fiber: 
        quando é  1: fiber ativa/pronta para ser executada;
        quando é  0: esperando outra fiber com join;
        quando é -1: fiber terminada;
    
    - fiberId: id inteiro da fiber. Nunca é negativo.
      o id da thread principal é 0.
    
    - joinFiber: ponteiro para a fiber que essa fiber 
      está esperando.
*/
typedef struct Fiber{
    struct Fiber * next;        // Próxima fiber da lista
    struct Fiber * prev;        // Fiber anterior
    ucontext_t * context;       // Contexto da fiber
    int status;                 // para fiberjoin, fiberexit e fiberdestroy
    fiber_t fiberId;            // Id da fiber
    struct Fiber * joinFiber;
    Waiting * waitingList;
}Fiber;

/*
    Struct da lista que armazenará as fibers criadas pela biblioteca.
    Atributos:

    - fibers: ponteiro para a "cabeça" da lista circular de fibers.
      A thread principal sempre será a "cabeça" da lista.

    - currentFiber: ponteiro para a fiber que está sendo executada 
      no momento.

    - nFibers: inteiro que contém o número de fibers na lista. A 
    thread principal faz parte da contagem.
*/
typedef struct FiberList{
    Fiber * fibers;             // Lista de fibers
    Fiber * currentFiber;       // FIber sendo executada no momento
    int nFibers;                // Quantidade de fibers na lista
}FiberList;

// Contexto para a função de escalonamento e da thread principal
ucontext_t schedulerContext, parentContext;

// Timer do escalonador
struct itimerval timer;

// Variáveis globais para o tempo do timer
int seconds = 0;
int microSeconds = 5000;

// Lista global que armazenará as fibers
FiberList * f_list = NULL;

/*
    timeHandler
    -----------

    Tratador do sinal SIGVTALRM, recebido sempre que o timer virtual é zerado.
    Serve apenas para salvar o contexto da fiber atual e trocar o contexto para
    a "fiber" do escalonador.


*/
void timeHandler(){
    if(swapcontext(f_list->currentFiber->context, &schedulerContext) == -1){
    	perror("Ocorreu um erro no swapcontext da timeHandler");
    	return;
    }
}

/*
    findFiber
    ---------

    Função que retorna o endereço de memória que guarda a fiber que corresponde
    ao id recebido "fiberId". Caso nenhuma fiber com esse id seja encontrada na
    lista, a função retornará NULL.
*/
Fiber * findFiber(fiber_t fiberId){

    // se for o id da thread principal, retorná-la
    if(f_list->fibers->fiberId == fiberId)
        return f_list->fibers;

    // se for o id da thread atual, retorná-la
    if(f_list->currentFiber->fiberId == fiberId)
        return f_list->currentFiber;
    
    // Obtendo a primeira fiber da lista de fibers
    Fiber * fiber = (Fiber *) f_list->fibers;
    int i;
    for(i = 0; i < f_list->nFibers; i++){ // encontrando a fiber com o id recebido
        fiber = (Fiber *) fiber->next;
        if(fiber->fiberId == fiberId)
            break;
    }

    // Caso a fiber não tenha sido encontrada
    if(i == f_list->nFibers) 
        return NULL;
    else return fiber;
}

void releaseFibers(Waiting *waitingList){
    // Enquanto houver fibers esperando
    while(waitingList != NULL){
        Fiber * waitingFiber = findFiber(waitingList->waitingId); // Procura a fiber com o id do nodo atual da waitingList
        if(waitingFiber != NULL && waitingFiber->status == 0) // Se a fiber existir e estiver esperando
            waitingFiber->status = 1;
        waitingList = waitingList->next; // procura
    }
}

/*
    Retorna a quantidade de fibers presentes na lista
*/
void getnFibers() {
    printf("%d\n", f_list->nFibers);
}

/*
    fiberScheduler
    --------------

    Toda vez que é chamada, verifica o status da fiber logo após a atual.

    Caso o status da próxima fiber não seja 1, o loop é executado e continuará
    procurando fibers disponíveis para execução. Fibers com status -1 nunca
    serão executadas, mas fibers com status 0 poderão ser executadas, caso
    a fiber que elas estiverem esperando esteja terminada(status -1), e terão
    seu status atualizado para 1 novamente.

    Caso a fiber tenha status 1, a fiber atual terá seu contexto salvo, e o 
    contexto atual será alterado para a fiber com status 1 encontrada.
*/
void fiberScheduler() {
    // Zerando o timer para para-lo
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;

    if(setitimer (ITIMER_VIRTUAL, &timer, NULL) == -1){
    	perror("Ocorreu um erro no setitimer da fiberScheduler");
    	return;
    }

    Fiber * nextFiber = (Fiber *) f_list->currentFiber->next;

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

    // Redefinindo o timer para o tempo normal
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = microSeconds;

    // Resetando o timer
    if(setitimer (ITIMER_VIRTUAL, &timer, NULL) == -1){
    	perror("Ocorreu um erro no setitimer da fiberScheduler");
    	return;
    }

    // Setando o contexto da próxima fiber
	if(setcontext(nextFiber->context) == -1){
    	perror("Ocorreu um erro no setcontext da fiberScheduler");
    	return;
    }
}

/*
    Função responsável por iniciar o escalonado
*/
void startFibers() {
    struct sigaction sa;

    // Definindo a primeira fiber como a fiber atual(a fiber logo após a thread principal)
    f_list->currentFiber = (Fiber *) f_list->fibers->next;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timeHandler;
    if(sigaction (SIGVTALRM, &sa, NULL) == -1){
    	perror("Ocorreu um erro no sigaction da startFibers");
    	return;
    }
 
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = microSeconds;
 
    timer.it_interval.tv_sec = seconds;
    timer.it_interval.tv_usec = microSeconds;

    if(setitimer (ITIMER_VIRTUAL, &timer, NULL) == -1){
    	perror("Ocorreu um erro no sititimer da startFibers");
    	return;
    }

    // Setando o contexto para o contexto da primeira fiber
    if(setcontext(f_list->currentFiber->context) == -1){
    	perror("Ocorreu um erro no setcontext da startFibers");
    	return;
    }
}

/*
    initFiberList
    -------------

    Função responsável por inicializar a lista de fibers caso ela não tenha
    sido inicializada ainda.
    Ela também insere a estrutura da thread principal como a cabeça da lista,
    e cria o contexto para a função FiberScheduler.
*/
int initFiberList() {
    
    f_list = (FiberList *) malloc(sizeof(FiberList));
    f_list->fibers = NULL;
    f_list->nFibers = 0;
    f_list->currentFiber = NULL;
    
    // Criando a estrutura de fiber para a thread principal
    Fiber * parentFiber = (Fiber *) malloc(sizeof(Fiber));

    // Inicializando a estrutura da thread principal
    parentFiber->context = &parentContext;
    parentFiber->fiberId = 0;
    parentFiber->prev = NULL;
    parentFiber->next = NULL; 
    parentFiber->status = 1;

    // Adicionado a estrutura da thread principal como o primeiro elemento da lista
    f_list->fibers = (Fiber *) parentFiber;

    // Incrementando o número de fibers da lista
    f_list->nFibers++;

    // Definindo a thread principal como fiber atual
    f_list->currentFiber = (Fiber *) parentFiber;

    // Obtendo um contexto para o escalonador
    if(getcontext(&schedulerContext) == -1){
    	perror("Ocorreu um erro no getcontext da initFiberList");
    	return 3;
    }
    
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

	return 0;
}

/*
    pushFiber
    ---------

    Insere a fiber recebida pela função na lista de fibers.
*/
void pushFiber(Fiber * fiber) {
    
    // Iniciando a lista de fibers, caso seja null
    if(f_list == NULL)
        initFiberList();
    
    // Obtendo a primeira fiber da lista (Processo pai)
    Fiber * firstFiber = (Fiber *) f_list->fibers;

    // Caso só haja a fiber da thread principal na lista
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
    if(getcontext(fiberContext) == -1){
    	perror("Ocorreu um erro no getcontext da fiber_create");
    	return 3;
    }

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
    fiberS->waitingList = NULL;


    // Inserindo a nova fiber na lista de fibers
    pushFiber(fiberS);

    // Atribuindo o id da fiber adequadamente
    * fiber = f_list->nFibers - 1;
    fiberS->fiberId = * fiber;

    //pegando o contexto da thread principal e passando para o parentcontext(assim
    // o atualizando sempre que a fiber_create for chamada)
    Fiber * parent = (Fiber *) f_list->fibers;
    if(getcontext(parent->context) == -1){
    	perror("Ocorreu um erro no getcontext da fiber_create");
    	return 5;
    }

    return 0;
}

/*
    fiber_join
    ----------

    Faz com que a thread atual espere o término de outra thread para começar a executar.

*/
int fiber_join(fiber_t fiber, void **retval){

    Fiber * fiberS = findFiber(fiber);

    // se a fiber não foi encontrada na lista
    if(fiberS == NULL)
        return -1;

    // se a fiber a ser esperada é a que está executando
    if(fiberS->fiberId == f_list->currentFiber->fiberId)
        return 1;

    // Se a fiber que deveria terminar antes já terminou, retornar normalmente
    if(fiberS->status == -1) {
        return 0;
    }

    // Marcando a fiber atual como esperando
    f_list->currentFiber->status = 0;

    // Definindo a fiber que a fiber atual está esperando
    f_list->currentFiber->joinFiber = (Fiber *) fiberS;

    // Trocando para o contexto do escalonador
    if(swapcontext(f_list->currentFiber->context, &schedulerContext) == -1){
    	perror("Ocorreu um erro no swapcontext da fiber_join");
    	return 3;
    }

    // Exibido quando a fiber que realizou o join volta a executar
    printf("join deu certo!!!! Id da thread que retornou: %d\n", f_list->currentFiber->fiberId);

    // Definindo o status da fiber atual como pronta para executar
    f_list->currentFiber->status = 1;
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

    if(f_list->currentFiber->fiberId == fiber){ //se a fiber atual tentar se destruir
        printf("Não é permitido que uma fiber destrua a si mesma");
        return 2;
    }

    // Obtendo a primeira fiber da lista de fibers e a fiber atual
    Fiber * fiberS = findFiber(fiber);

    // Caso a fiber não tenha sido encontrada
    if(fiberS == NULL) 
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

    Para a execução da fiber atual e altera seu status para -1.
    Com seu status -1, essa fiber nunca mais será executada.
    Logo após isso, o escalonador é chamado.

*/
void fiber_exit(void *retval){

    // Caso a fiber atual seja a thread principal
    if (f_list->currentFiber->fiberId == 0){
        printf("Uso inadequado de fiber_exit(). Caso queira terminar o programa, use exit() em vez disso.\n");
        return;
    }

    // Definindo status da fiber atual como terminada
    f_list->currentFiber->status = -1;

    // Percorrendo a lista de fibers em espera e as liberando
    releaseFibers(f_list->currentFiber->waitingList);

    // Chamando o escalonador(essa fiber não irá voltar)
    fiberScheduler();
}
