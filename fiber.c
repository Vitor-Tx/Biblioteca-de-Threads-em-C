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

// Id da thread principal
#define PARENT_ID -1

// Timeslice das fibers
#define SECONDS 0
#define MICSECONDS 35000

// Status das fibers
#define READY 1
#define WAITING 0
#define FINISHED -1

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
    fiber_t waitingId;       // Id da fiber do nodo atual
    struct Waiting * next;   // Ponteiro para o próximo nodo
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
    
    - fiberId: id inteiro da fiber.
      O id da thread principal é PARENT_ID.
    
    - joinFiber: ponteiro para a fiber que essa fiber 
      está esperando.

    - waitingList: lista com id's das fibers que estão esperando
      essa fiber em joins.
*/
typedef struct Fiber{
    struct Fiber * next;        // Próxima fiber da lista
    struct Fiber * prev;        // Fiber anterior
    ucontext_t context;       // Contexto da fiber
    int status;                 // Status atual da fiber
    fiber_t fiberId;            // Id da fiber
    void * retval;              // valor de retorno da fiber
    void * join_retval;        // valor de retorno da fiber que ela estava esperando
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
    int started;                // Indica se as fibers estão rodando
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
    if(swapcontext(&f_list->currentFiber->context, &schedulerContext) == -1){
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
    if(restored != NULL) // se restored for NULL, não é pra restaurar
        if (getitimer(ITIMER_VIRTUAL, restored) == -1) {
            perror("erro na getitimer() da stopTimer");
            exit(1);
        }
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
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
    // caso não haja lista de fibers ainda
    if(f_list == NULL)
        return NULL;
    // se não estiver inicializado
    if(fiberId == 0)
        return NULL;
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
        // Se a fiber existir e estiver esperando
        if(waitingFiber != NULL && waitingFiber->status == WAITING){
            waitingFiber->status = READY;  // libera a fiber
            waitingFiber->join_retval = waitingFiber->joinFiber->retval; // guarda o retval
        } 
        free(waitingList); // libera o nodo no topo
        waitingList = (Waiting *) waitingNode; // vai para o próximo nodo
    }
    
}

/*
    fiber_destroy
    -------------

    Destrói uma fiber struct apontada por "fiber" e 
    reorganiza os nodos da lista de fibers.

*/
Fiber * fiber_destroy(Fiber * fiber){

    int id = fiber->fiberId;

    //apenas fibers terminadas podem ser destruídas
    if(fiber->status != FINISHED)
		return NULL; 
    
    //pegando a fiber anterior a fiber que vai ser destruída
    Fiber * prevFiber = (Fiber *) fiber->prev;

    //pegando a proxima fiber da fiber que vai ser destruída
    Fiber * nextFiber = (Fiber *) fiber->next;

	if(nextFiber != NULL && prevFiber != NULL){
		// tirando a fiber que irá ser destruida da lista circular
		nextFiber->prev = (Fiber *) prevFiber; 
		prevFiber->next = (Fiber *) nextFiber;
	}    

	if(id == PARENT_ID)
		f_list->fibers = nextFiber;
	
    //destruindo a fiber
    free(fiber->context.uc_stack.ss_sp);
    free(fiber);
	fiber = NULL;

    f_list->nFibers--;

	// preenchendo ponteiros com NULL para evitar acesso indevido de memória
	if(f_list->nFibers == 0){
		nextFiber = NULL;
		prevFiber = NULL;
	}
	
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
    stopTimer(NULL);

    Fiber * nextFiber = (Fiber *) f_list->currentFiber->next;
    
    while(nextFiber->status != READY) { // pulando a thread atual do loop não estiver pronta pra executar
        // Caso a fiber já tenha terminado
        if(nextFiber->status == FINISHED){
            // liberando as fibers esperando esta(caso existam)
            releaseFibers(nextFiber->waitingList);
            // destruindo essa fiber e obtendo a próxima(caso exista)
            nextFiber = fiber_destroy(nextFiber);
            // se a fiber_destroy retornar NULL
            if(nextFiber == NULL){
				// caso não haja mais nenhuma fiber na lista
                if(f_list->nFibers == 0){
                    free(f_list); // liberando a lista de fibers
					free(schedulerContext.uc_stack.ss_sp); // liberando a pilha do escalonador
                    exit(0); // terminando o programa
                }
				// caso contrário, algum erro ocorreu
				exit(-1); 
            }
        } 
        
        // Caso a thread atual estiver num join
        if(nextFiber->status == WAITING) { 
            // Caso a thread que ela está esperando não tiver terminado
            if(nextFiber->joinFiber->status != FINISHED)
                nextFiber = (Fiber *) nextFiber->next; // pula a thread que está esperando
            else // Caso a thread que ela está esperando tenha terminado, setar o status para READY
				nextFiber->status = READY; 
            
        }
    } 
        
    // Definindo a próxima fiber selecionada como a fiber atual
	f_list->currentFiber = (Fiber *) nextFiber;

    // Redefinindo o timer para o tempo normal
    timer.it_value.tv_sec = SECONDS;
    timer.it_value.tv_usec = MICSECONDS;

    // Resetando o timer
    restoreTimer(&timer);

    // Setando o contexto da próxima fiber
	if(setcontext(&nextFiber->context) == -1){
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
    f_list->started = 0;
    
    // Criando a estrutura de fiber para a thread principal
    Fiber * parentFiber = (Fiber *) malloc(sizeof(Fiber));
    if (parentFiber == NULL) {
        perror("erro malloc na criação da parentFiber da initFiberList");
        return ERR_MALL;
    }

    // Inicializando a estrutura da thread principal
    parentFiber->context = parentContext;
    parentFiber->fiberId = PARENT_ID;
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
    
    // Limpando o contexto recebido para o escalonador
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

	// Incrementando o número de fibers
    f_list->nFibers++;
	
    // restaura o timer para o restante do timeslice que a fiber atual possuía
    restoreTimer(&restored);

    
}

/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine recebendo o parâmetro arg.

*/
int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg) {
    
    // se o ponteiro apontar para NULL
    if(fiber == NULL)
        return ERR_NULLID;

    // verificando se já existe uma fiber com esse id
    if(findFiber(* fiber) != NULL){
        printf("Essa fiber já existe\n");
        return ERR_EXISTS;
    }

    // Variável que irá armazenar o contexto da nova fiber
    ucontext_t fiberContext;

    // Struct que irá armazenar a nova fiber
    Fiber * fiberNode = (Fiber *) malloc(sizeof(Fiber));
    if (fiberNode == NULL) {
        perror("erro malloc na criação da fiber struct da fiber_create");
        return ERR_MALL;
    }
    
    // Obtendo o contexto atual e armazenando-o na variável fiberContext
    if(getcontext(&fiberContext) == -1){
    	perror("Ocorreu um erro no getcontext da fiber_create");
    	return ERR_GTCTX;
    }

    // Modificando o contexto para uma nova pilha
    fiberContext.uc_link = &schedulerContext;
    fiberContext.uc_stack.ss_sp = malloc(FIBER_STACK);
    fiberContext.uc_stack.ss_size = FIBER_STACK;
    fiberContext.uc_stack.ss_flags = 0;        
    if (fiberContext.uc_stack.ss_sp == 0 ) {
        perror("erro malloc na criação da pilha na fiber_create");
        return ERR_MALL;
    }

    // Criando a fiber propriamente dita
    makecontext(&fiberContext, (void (*)(void )) start_routine, 1, arg);

    // Inicializando a struct que armazena a fiber recem criada
    fiberNode->context = fiberContext;
    fiberNode->prev = NULL;
    fiberNode->next = NULL;
    fiberNode->status = READY;
    fiberNode->retval = NULL;
    fiberNode->join_retval = NULL;
    fiberNode->joinFiber = NULL;
    fiberNode->waitingList = NULL;

    // Inserindo a nova fiber na lista de fibers
    pushFiber(fiberNode);

    // Atribuindo o id da fiber adequadamente
    * fiber = f_list->nFibers - 1;
    fiberNode->fiberId = * fiber;

    // Verificando se o escalonador já começou a rodar.
    // Caso não tenha, startFibers() é chamada e o contexto
    // da thread principal é capturado.
    if (f_list->started == 0) {
        f_list->started = 1;
        startFibers();

        //pegando o contexto da thread atual e passando para o currentContext(assim
        // o atualizando sempre que a fiber_create for chamada)
        if(getcontext(&f_list->fibers->context) == -1){
            perror("Ocorreu um erro no getcontext da fiber_create");
            return ERR_GTCTX;
        }
    }

    return 0;
}

/*
    fiber_join
    ----------

    Faz com que a thread atual espere o término de outra thread para começar a executar.

*/
int fiber_join(fiber_t fiber, void **retval){

    Fiber * fiberNode = findFiber(fiber);

    // se a fiber não foi encontrada na lista
    if(fiberNode == NULL)
        return ERR_NOTFOUND;

    // se a fiber a ser esperada é a que está executando
    if(fiberNode->fiberId == f_list->currentFiber->fiberId)
        return ERR_JOINCRRT;

    // Se a fiber que deveria terminar antes já terminou, retornar normalmente
    if(fiberNode->status == FINISHED) {
        return 0;
    }

	// criando um nodo para a lista de espera da fiber que será aguardada
    Waiting * waitingNode = (Waiting *) malloc(sizeof(Waiting));
    if (waitingNode == NULL) {
        perror("erro malloc na fiber_join");
        return ERR_MALL;
    }
    waitingNode->waitingId = fiber;
    waitingNode->next = NULL;

    // parar o timer, área crítica
    stopTimer(NULL);

    // adicionando um nodo na lista de espera da fiber a ser aguardada
    if(fiberNode->waitingList == NULL){
        fiberNode->waitingList = (Waiting *) waitingNode;     
    }  
    else{
        Waiting * waitingTop = (Waiting *) fiberNode->waitingList;
        fiberNode->waitingList = (Waiting *) waitingNode;
        fiberNode->waitingList->next = (Waiting *) waitingTop;
    }

    // Definindo a fiber que a fiber atual está esperando
    f_list->currentFiber->joinFiber = (Fiber *) fiberNode;

    // Marcando a fiber atual como esperando
    f_list->currentFiber->status = WAITING;  

    // Trocando para o contexto do escalonador
    if(swapcontext(&f_list->currentFiber->context, &schedulerContext) == -1){
    	perror("Ocorreu um erro no swapcontext da fiber_join");
    	return ERR_SWPCTX;
    }

    // Recuperando o valor de retorno da fiber que estava sendo aguardada.
	// Caso NULL tenha sido passado como argumento para retval, nada mais é feito.
    // Caso a fiber que estava sendo aguardada por esta tenha sido destruída,
    // as rotinas de destruição já distribuíram os valores de retval corretamente
    // para os atributos join_retval das fibers que estavam aguardando-a.
	if(retval != NULL){
		if(f_list->currentFiber->join_retval == NULL && f_list->currentFiber->joinFiber != NULL)
			*retval = f_list->currentFiber->joinFiber->retval;
		else 
			*retval = f_list->currentFiber->join_retval;
		
		// resetando os retvals da fiber
		f_list->currentFiber->retval = NULL;
		f_list->currentFiber->join_retval = NULL;
	}

    // Definindo o status da fiber atual como pronta para executar
    f_list->currentFiber->status = READY;
    return 0;
}

/*
    fiber_exit
    ----------

    Para a execução da fiber atual e altera seu status para FINISHED.
    Com seu status FINISHED, essa fiber nunca mais será executada.
    Logo após isso, o escalonador é chamado.

*/
void fiber_exit(void *retval){

    // Setando o retval da fiber
    f_list->currentFiber->retval = retval;
    // Definindo status da fiber atual como terminada
    f_list->currentFiber->status = FINISHED;

    // Chamando o escalonador corretamente
    timeHandler();
}
