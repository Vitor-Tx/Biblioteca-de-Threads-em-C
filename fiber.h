typedef unsigned int fiber_t;


/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine.

*/
int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg);

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
