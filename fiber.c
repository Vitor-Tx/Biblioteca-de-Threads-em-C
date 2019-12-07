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

// Timeslice das fibers
#define SECONDS 0
#define MICSECONDS 35000

// Status das fibers
#define READY 1
#define WAITING 0
#define FINISHED -1

// Erros das funções
#define ERR_EXISTS 11
#define ERR_MALL 22
#define ERR_GTCTX 33
#define ERR_SWPCTX 44
#define ERR_NOTFOUND 55
#define ERR_JOINCRRT 66

typedef unsigned int fiber_t; // tipo para ID de fibers

/*
    Waiting
    -------

    Struct que representa um nodo de uma lista de id's de fibers que estão esperando uma fiber. 
    ******************************************************************************************

    Atributos:
    +++++++++

    - waitingId: Id da fiber desse nodo que está esperando.
    
    - next: ponteiro para o próximo nodo da lista.

*/
typedef struct Waiting{
    fiber_t waitingId;              // Id da fiber do nodo atual
    struct Waiting * next;          // Ponteiro para o próximo nodo
}Waiting;

/*
    Fiber
    -----

    Struct de uma fiber(thread de user-level).
    *****************************************

    Atributos:
    +++++++++

    - next e prev: ponteiros para outras estruturas
      de fibers para que seja feita uma lista circular.

    - context: ponteiro para a estrutura ucontext_t
      que contém o contexto da fiber.

    - status: representa o estado atual da fiber: 
        READY: fiber ativa/pronta para ser executada;
        WAITING: esperando outra fiber com join;
        FINISHED: fiber terminada;
    
    - fiberId: id inteiro da fiber. Nunca é negativo.
      o id da thread principal é 0.
    
    - joinFiber: ponteiro para a fiber que essa fiber 
      está esperando.

    - waitingList: lista com id's das fibers que estão esperando
      essa fiber em joins.
*/
typedef struct Fiber{
    struct Fiber * next;        // Próxima fiber da lista
    struct Fiber * prev;        // Fiber anterior
    ucontext_t * context;       // Contexto da fiber
    int status;                 // Status atual da fiber
    fiber_t fiberId;            // Id da fiber
    struct Fiber * joinFiber;   // Ponteiro para a fiber que essa fiber está esperando
    Waiting * waitingList;      // Lista de fibers que estão esperando essa fiber
}Fiber;

/*
    FiberList
    ---------

    Struct da lista que armazenará as fibers criadas pela biblioteca.
    ****************************************************************

    Atributos:
    +++++++++

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
    stopTimer
    ------------

    Salva os segundos e microsegundos restantes do timer atual
    na estrutura apontada por restored, e para o timer atual.

*/
void stopTimer(struct itimerval * restored){
    if (getitimer(ITIMER_VIRTUAL, restored) == -1) {
        perror("erro na getitimer() da stopTimer");
        exit(1);
    }
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_sec = 0;
    if(setitimer (ITIMER_VIRTUAL, &timer, NULL) == -1){
    	perror("Ocorreu um erro no setitimer() da stopTimer");
    	return;
    }
}

/*
    restoreTimer
    ------------

    Seta o timer com os valores obtidos pela estrutura apontada
    por restored.

*/
void restoreTimer(struct itimerval * restored){
    // Restaurando o timer para o restante do timeslice
    if(setitimer (ITIMER_VIRTUAL, restored, NULL) == -1){
    	perror("Ocorreu um erro no setitimer da restoreTimer");
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

/*
    releaseFibers
    -------------

    Função que libera todas as fibers da waitingList num join.
*/
void releaseFibers(Waiting *waitingList){
    // Enquanto houver fibers esperando
    while(waitingList != NULL){
        Waiting * waitingNode = waitingList->next;
        Fiber * waitingFiber = findFiber(waitingList->waitingId); // Procura a fiber com o id do nodo atual da waitingList
        if(waitingFiber != NULL && waitingFiber->status == WAITING) // Se a fiber existir e estiver esperando
            waitingFiber->status = READY;  // libera a fiber
        free(waitingList); // libera o nodo no topo
        waitingList = (Waiting *) waitingNode; // vai para o próximo nodo
    }
    
}

/*
    fiber_destroy
    -------------

    Destrói uma fiber apontada por fiber e reorganiza
    os nodos da lista de fibers.

*/
Fiber * fiber_destroy(Fiber * fiber){

    int id = fiber->fiberId;

    if(id == 0) // não é permitido destruir a thread principal
        return NULL;
    
    if(f_list->currentFiber->fiberId == id){ //se a fiber atual tentar se destruir
        printf("Não é permitido que uma fiber destrua a si mesma");
        return NULL;
    }
    
    if(fiber->status != FINISHED) //apenas fibers terminadas podem ser destruídas
        return NULL;
    
    //pegando a fiber anterior a fiber que vai ser destruida
    Fiber * prevFiber = (Fiber *) fiber->prev;
    if(prevFiber == NULL)
        printf("prevFiber é null\n");

    //pegando a proxima fiber da fiber que vai ser destruida
    Fiber * nextFiber = (Fiber *) fiber->next;
    if(nextFiber == NULL)
        printf("nextFiber é null\n");

    // tirando a fiber que irá ser destruida da lista circular
    nextFiber->prev = (Fiber *) prevFiber; 

    prevFiber->next = (Fiber *) nextFiber;
    
    //destruindo a fiber
    free(fiber->context->uc_stack.ss_sp);
    free(fiber->context);
    free(fiber);

    f_list->nFibers--;
    // (remover depois)
    // printf("Fiber destruída com sucesso, o id dela era: %d\n", id);

    return nextFiber;
}

/*
    getnFibers
    ----------

    Retorna a quantidade de fibers presentes na lista.
*/
void getnFibers() {
    printf("%d\n", f_list->nFibers);
}

/*
    fiberScheduler
    --------------

    Toda vez que é chamada, verifica o status da fiber logo após a atual.

    Caso o status da próxima fiber não seja READY, o loop é executado e continuará
    procurando fibers disponíveis para execução. Fibers com status FINISHED nunca
    serão executadas, mas fibers com status WAITING poderão ser executadas, caso
    a fiber que elas estiverem esperando esteja terminada(status FINISHED), e terão
    seu status atualizado para READY novamente.

    Caso a fiber tenha status READY, a fiber atual terá seu contexto salvo, e o 
    contexto atual será alterado para a fiber com status READY encontrada.
*/
void fiberScheduler() {
    // Zerando o timer para pará-lo
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;

    if(setitimer (ITIMER_VIRTUAL, &timer, NULL) == -1){
    	perror("Ocorreu um erro no setitimer da fiberScheduler");
    	return;
    }

    Fiber * nextFiber = (Fiber *) f_list->currentFiber->next;

    while(nextFiber->status != READY) { // pulando a thread atual do loop não estiver pronta pra executar

        // Caso a fiber já tenha terminado
        if(nextFiber->status == FINISHED){
            
            releaseFibers(nextFiber->waitingList);
            nextFiber = fiber_destroy(nextFiber);
            if(nextFiber == NULL){
                printf("Erro na fiber_destroy.\n");
            }
        } 
        
        // Caso a thread atual estiver num join
        if(nextFiber->status == WAITING) { 

            // Caso a thread que ela está esperando não tiver terminado
            if(nextFiber->joinFiber->status != FINISHED) {
                nextFiber = (Fiber *) nextFiber->next; // pula a thread que está esperando
            } 
            
            else {

                // Caso a thread que ela está esperando tenha terminado
                nextFiber->status = READY;
            }
        }
    } 
        
    // Definindo a próxima fiber selecionada como a fiber atual
	f_list->currentFiber = (Fiber *) nextFiber;

    // Redefinindo o timer para o tempo normal
    timer.it_value.tv_sec = SECONDS;
    timer.it_value.tv_usec = MICSECONDS;

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
    startFibers
    -----------

    Função responsável por inicializar as estruturas do timer e 
    sinalizador, além de fazer a primeira chamada para o escalonador,
    assim começando o ciclo de escalonamento de fibers.
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
 
    timer.it_value.tv_sec = SECONDS;
    timer.it_value.tv_usec = MICSECONDS;
 
    timer.it_interval.tv_sec = SECONDS;
    timer.it_interval.tv_usec = MICSECONDS;

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
    if (f_list == NULL) {
        perror("erro malloc na criação da lista na initFiberList");
        return ERR_MALL;
    }
    f_list->fibers = NULL;
    f_list->nFibers = 0;
    f_list->currentFiber = NULL;
    
    // Criando a estrutura de fiber para a thread principal
    Fiber * parentFiber = (Fiber *) malloc(sizeof(Fiber));
    if (parentFiber == NULL) {
        perror("erro malloc na criação da parentFiber da initFiberList");
        return ERR_MALL;
    }

    // Inicializando a estrutura da thread principal
    parentFiber->context = &parentContext;
    parentFiber->fiberId = 0;
    parentFiber->prev = NULL;
    parentFiber->next = NULL; 
    parentFiber->status = READY;

    // Adicionado a estrutura da thread principal como o primeiro elemento da lista
    f_list->fibers = (Fiber *) parentFiber;

    // Incrementando o número de fibers da lista
    f_list->nFibers++;

    // Definindo a thread principal como fiber atual
    f_list->currentFiber = (Fiber *) parentFiber;

    // Obtendo um contexto para o escalonador
    if(getcontext(&schedulerContext) == -1){
    	perror("Ocorreu um erro no getcontext da initFiberList");
    	return ERR_GTCTX;
    }
    
    // Limpamdo o contexto recebido para o escalonador
    schedulerContext.uc_link = &parentContext;
    schedulerContext.uc_stack.ss_sp = malloc(FIBER_STACK);
    schedulerContext.uc_stack.ss_size = FIBER_STACK;
    schedulerContext.uc_stack.ss_flags = 0;        
    if (schedulerContext.uc_stack.ss_sp == NULL) {
        perror("erro malloc na criação da pilha na initFiberList");
        return ERR_MALL;
    }

    // Criando o contexto do escalonador própriamente dito
    makecontext(&schedulerContext, fiberScheduler, 1, NULL);

	return 0;
}

/*
    pushFiber
    ---------

    Insere a fiber recebida pela função na lista de fibers. Caso a 
    lista ainda não tenha sido criada, a função initFiberList() é 
    chamada. Durante a inserção da fiber, o timeslice restante da 
    fiber atual é salvo e o timer é pausado. Depois que a inserção
    é feita, o timer é reiniciado com o timeslice salvo anteriormente.

*/
void pushFiber(Fiber * fiber) {
    
    // Iniciando a lista de fibers, caso seja null
    if(f_list == NULL)
        initFiberList();

    struct itimerval restored;

    //para o timer para evitar troca de fibers em região crítica
    stopTimer(&restored);
    // Caso só haja a fiber da thread principal na lista
    if (f_list->nFibers == 1) {
        fiber->prev = (Fiber *) f_list->fibers;
        fiber->next = f_list->fibers;
        f_list->fibers->prev = (Fiber *) fiber; 
        f_list->fibers->next = (Fiber *) fiber;
        
    } else {
        // Caso haja outras fibers na lista
        fiber->next = (Fiber *) f_list->fibers;
        fiber->prev = (Fiber *) f_list->fibers->prev;
        fiber->prev->next = (Fiber *) fiber;
        f_list->fibers->prev = (Fiber *) fiber;
    }

    // restaura o timer para o restante do timeslice que a fiber atual possuía
    restoreTimer(&restored);

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

    if((* fiber) != NULL){
        printf("Essa fiber já existe\n");
        return ERR_EXISTS;
    }

    // Variável que irá armazenar a nova fiber 
    ucontext_t * fiberContext = (ucontext_t *) malloc(sizeof(ucontext_t));
    if (fiberContext == NULL) {
        perror("erro malloc na criação do context da fiber_create");
        return ERR_MALL;
    }

    // Struct que irá armazenar a nova fiber
    Fiber * fiberS = (Fiber *) malloc(sizeof(Fiber));
    if (fiberS == NULL) {
        perror("erro malloc na criação da fiber struct da fiber_create");
        return ERR_MALL;
    }
    
    // Obtendo o contexto atual e armazenando-o na variável fiberContext
    if(getcontext(fiberContext) == -1){
    	perror("Ocorreu um erro no getcontext da fiber_create");
    	return ERR_GTCTX;
    }

    // Modificando o contexto para uma nova pilha
    fiberContext->uc_link = &schedulerContext;
    fiberContext->uc_stack.ss_sp = malloc(FIBER_STACK);
    fiberContext->uc_stack.ss_size = FIBER_STACK;
    fiberContext->uc_stack.ss_flags = 0;        
    if (fiberContext->uc_stack.ss_sp == 0 ) {
        perror("erro malloc na criação da pilha na fiber_create");
        return ERR_MALL;
    }

    // Criando a fiber propriamente dita
    makecontext(fiberContext, (void (*)(void )) start_routine, 1, arg);

    // Inicializando a struct que armazena a fiber recem criada
    fiberS->context = fiberContext;
    fiberS->prev = NULL;
    fiberS->next = NULL;
    fiberS->status = READY;
    fiberS->joinFiber = NULL;
    fiberS->waitingList = NULL;

     

    // Inserindo a nova fiber na lista de fibers
    pushFiber(fiberS);

    // Atribuindo o id da fiber adequadamente
    * fiber = f_list->nFibers - 1;
    fiberS->fiberId = * fiber;

    //pegando o contexto da thread principal e passando para o parentcontext(assim
    // o atualizando sempre que a fiber_create for chamada)
    if(getcontext(f_list->currentFiber->context) == -1){
    	perror("Ocorreu um erro no getcontext da fiber_create");
    	return ERR_GTCTX;
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
        return ERR_NOTFOUND;

    // se a fiber a ser esperada é a que está executando
    if(fiberS->fiberId == f_list->currentFiber->fiberId)
        return ERR_JOINCRRT;

    // Se a fiber que deveria terminar antes já terminou, retornar normalmente
    if(fiberS->status == FINISHED) {
        return 0;
    }

    Waiting * waitingNode = (Waiting *) malloc(sizeof(Waiting));
    if (waitingNode == NULL) {
        perror("erro malloc na fiber_join");
        return ERR_MALL;
    }
    waitingNode->waitingId = fiber;
    waitingNode->next = NULL;


    if(fiberS->waitingList == NULL){
        fiberS->waitingList = (Waiting *) waitingNode;     
    }  
    else{
        Waiting * waitingTop = (Waiting *) fiberS->waitingList;
        fiberS->waitingList = (Waiting *) waitingNode;
        fiberS->waitingList->next = (Waiting *) waitingTop;
    }

    // Definindo a fiber que a fiber atual está esperando
    f_list->currentFiber->joinFiber = (Fiber *) fiberS;

    // Marcando a fiber atual como esperando
    f_list->currentFiber->status = WAITING;  

    // Trocando para o contexto do escalonador
    if(swapcontext(f_list->currentFiber->context, &schedulerContext) == -1){
    	perror("Ocorreu um erro no swapcontext da fiber_join");
    	return ERR_SWPCTX;
    }

    // (remover depois) Exibido quando a fiber que realizou o join volta a executar
    // printf("join deu certo!!!! Id da thread que retornou: %d\n", f_list->currentFiber->fiberId);

    // Definindo o status da fiber atual como pronta para executar
    f_list->currentFiber->status = READY;
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
    f_list->currentFiber->status = FINISHED;

    // Chamando o escalonador(essa fiber não irá voltar)
    fiberScheduler();
}
