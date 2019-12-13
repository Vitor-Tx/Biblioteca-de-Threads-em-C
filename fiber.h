/*
                                       
    _____________  _____  ______________    _______________  _______________      _____              _____  _______________
    |\\//\\//\\||  |\\||  |//\\//\\//\/\\   |\\//\\//\\//||  |//\\//\\//\\/\\     |\\||              |\\||  |\\//\\//\\//\\\    
    |//||          |//||  |\\||       \/\\  |//\\//\\//\\||  |\\||       \\/\\    |//||              |//||  |//||        \/\\
    |\\||          _____  |//||       |/||  |\\||            |//||       |/\||    |\\||              _____  |\\||        |\||
    |//||________  |//||  |\\||_______/\//  |//||__________  |\\||_______/\///    |//||              |//||  |//||________/\//
    |\\//\\//\\||  |\\||  |//\\//\\//\\/||  |\\//\\//\\//||  |//\\//\\//\\///     |\\||              |\\||  |\\//\\//\\//\\||
    |//||          |//||  |\\||       \/\\  |//\\//\\//\\||  |\\||      \/\\\     |//||              |//||  |//||        \/\\
    |\\||          |\\||  |//||       |\||  |\\||__________  |//||       \//\\    |//||____________  |\\||  |\\||        |\||
    |//||          |//||  |\\||_______/\//  |//\\//\\//\\||  |\\||        \\/\\   |//\\//\\//\\//||  |//||  |//||________/\//                  
    |\\||          |\\||  |//\\//\\//\\//   |\\//\\//\\//||  |//||         \\/\\  |\\//\\//\\//\\||  |\\||  |\\//\\//\\//\//
    ´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´                                                                        

    FiberLib
    --------

    Implementação de threads em user-level(fibers) no modelo M para 1(M threads user-level para 1 
    thread kernel-level) com escalonamento preemptivo utilizando o algoritmo round-robin.

    A alocação de memória para ponteiros que guardam e recebem valores de retorno de fibers é de 
    TOTAL RESPONSABILIDADE DOS USUÁRIOS DA BIBLIOTECA. Além disso, as rotinas aqui implementadas
    NÃO EVITAM que recursos possam ser acessados por múltiplas fibers ao mesmo tempo, nem 
    mesmo evitam que o processo se bloqueie devido a joins encadeados.

    by Guilherme Bartasson, Diego Batistuta e Vitor Teixeira, 2019
*/

typedef int fiber_t; // tipo para ID de fibers

// Erros das funções
#define ERR_EXISTS   11
#define ERR_MALL     22
#define ERR_GTCTX    33
#define ERR_SWPCTX   44
#define ERR_NOTFOUND 55
#define ERR_JOINCRRT 66
#define ERR_NULLID   77

/*
    fiber_create
    ------------

    Cria uma fiber(user-level thread) que executará a rotina(função)
    start_routine, recebendo o parâmetro arg. Caso a fiber seja criada
    corretamente, ela será inserida na lista de fibers, e seu id será
    transferido para o endereço apontado por *fiber.

*/
int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg);

/*
    fiber_join
    ----------

    Faz com que a fiber atual espere o término de outra fiber para começar a executar.
    Caso o ponteiro recebido por retval não seja NULL, a função transferirá o endereço
    de memória do valor de retorno da fiber que estava sendo aguardada para ele, para que
    este possa permitir que o usuário da biblioteca recupere esse valor de retorno e o use.

    A alocação de memória necessária para o ponteiro duplo(void ** retval) é de total
    responsabilidade do usuário da biblioteca.

*/
int fiber_join(fiber_t fiber, void **retval);

/*
    fiber_exit
    ----------

    Para a execução da fiber atual e altera seu status para FINISHED.
    Com seu status FINISHED, essa fiber nunca mais será executada, e
    ao ser identificada pelo escalonador, terá sua área de memória
    liberada, após ter corretamente transferido o endereço de memória
    de retval para as fibers que a estavam esperando e as liberado para
    serem executadas pelo escalonador, caso existam.

    A alocação de memória necessária para o ponteiro retval é de total
    responsabilidade do usuário da biblioteca, e caso um valor nulo 
    seja recebido em seu lugar, esse valor será simplesmente ignorado
    pelas rotinas que fazem uso dele.

*/
void fiber_exit(void *retval);
