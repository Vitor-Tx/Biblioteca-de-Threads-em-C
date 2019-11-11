# TrabalhodeSO
Trabalho da diciplina Sistemas Operacionais do curso de Sistemas de Informação da UFU

As características da biblioteca FiberLib seguem:

 A FiberLib deve implementar toda a infraestrutura para a criação de threads no user-space, seguindo o
modelo M:1, ou seja, M threads no user space mapeadas para uma thread no kernel space.

 A FiberLib deve implementar escalonamento preemptivo, baseado em time slice (algoritmo roundrobin).

 O código do alocador deve estar no formato de shared library no Linux.

 Tal como a biblioteca pthread, a FiberLib deve oferecer a seguinte interface mínima:

Arquivo <fiber.h> contendo todos símbolos necessários ao uso das rotinas da biblioteca.

Rotinas:
int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg);

int fiber_join(fiber_t fiber, void **retval);

int fiber_destroy(fiber_t fiber);

void fiber_exit(void *retval);

A semântica dessas rotinas segue aquela definida para as rotinas equivalentes da biblioteca pthread (vide
man pages).

Para implementar as rotinas supracitadas, bem como outras acessórias, será necessário o uso de várias
system calls, em especial: getcontext(), setcontext(), makecontext(), swapcontext(), sigaction() e
setitimer(). Consulte o manual online do Linux (man pages), bem como fontes complementares, para o
entendimento de cada uma dessas funções.
