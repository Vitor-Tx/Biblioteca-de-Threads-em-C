# TrabalhodeSO
Trabalho da disciplina Sistemas Operacionais do curso de Sistemas de Informação da UFU

As características da biblioteca FiberLib seguem:
<ul>
  <li>A FiberLib deve implementar toda a infraestrutura para a criação de threads no user-space, seguindo o modelo M:1, ou seja, M threads no user space mapeadas para uma thread no kernel space.</li>
  <li>A FiberLib deve implementar escalonamento preemptivo, baseado em time slice (algoritmo roundrobin).</li>
  <li>O código do alocador deve estar no formato de shared library no Linux.</li>
</ul>

<h3>Tal como a biblioteca pthread, a FiberLib deve oferecer a seguinte interface mínima:</h3>

<ul>
  <li>Arquivo <fiber.h> contendo todos símbolos necessários ao uso das rotinas da biblioteca.</li>
</ul>

<h3>Rotinas:</h3>

<ul>
  <li>int fiber_create(fiber_t *fiber, void *(*start_routine) (void *), void *arg);</li>
  <li>int fiber_join(fiber_t fiber, void **retval);</li>
  <li>int fiber_destroy(fiber_t fiber);</li>
  <li>void fiber_exit(void *retval);</li>
</ul>

A semântica dessas rotinas segue aquela definida para as rotinas equivalentes da biblioteca pthread (vide
man pages).

Para implementar as rotinas supracitadas, bem como outras acessórias, será necessário o uso de várias
system calls, em especial: <strong>getcontext()</strong>, <strong>setcontext()</strong>, <strong>makecontext()</strong>, <strong>swapcontext()</strong>, <strong>sigaction()</strong> e
<strong>setitimer()</strong>. Consulte o manual online do Linux (man pages), bem como fontes complementares, para o
entendimento de cada uma dessas funções.

## Links Úteis
<ul>
  <li><strong><span>Artigo sobre a implementação de fibers: </span></strong>https://www.evanjones.ca/software/threading.html</li>
  <li><strong><span>Função setcontext(): </span></strong>https://www.tutorialspoint.com/unix_system_calls/setcontext.htm</li>
</ul>
