typedef unsigned int fiber_t; // tipo para ID de fibers

// Erros das funções
#define ERR_EXISTS   11
#define ERR_MALL     22
#define ERR_GTCTX    33
#define ERR_SWPCTX   44
#define ERR_NOTFOUND 55
#define ERR_JOINCRRT 66
#define ERR_NULLID   77

void getnFibers();

/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine recebendo o parâmetro arg.

*/
int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg);

/*
    fiber_join
    ----------

    Faz com que a thread atual espere o término de outra thread para começar a executar.

*/
int fiber_join(fiber_t fiber, void **retval);

/*
    fiber_exit
    ----------

    Para a execução da fiber atual e altera seu status para FINISHED.
    Com seu status FINISHED, essa fiber nunca mais será executada.
    Logo após isso, o escalonador é chamado.

*/
void fiber_exit(void *retval);
