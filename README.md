
                                       
    _______________  _____  ______________    _______________  _______________      _____              _____  ______________
    |\\//\\//\\//||  |\\||  |//\\//\\//\/\\   |\\//\\//\\//||  |//\\//\\//\\/\\     |\\||              |\\||  |//\\//\\//\/\\     
    |//\\//\\//\\||  |//||  |\\||       \/\\  |//\\//\\//\\||  |\\||       \\/\\    |//||              |//||  |\\||       \/\\
    |\\||            _____  |//||       |/||  |\\||            |//||       |/\||    |\\||              _____  |//||       |/||
    |//||__________  |//||  |\\||_______/\//  |//||__________  |\\||_______/\///    |//||              |//||  |\\||_______/\//
    |\\//\\//\\//||  |\\||  |//\\//\\//\\/||  |\\//\\//\\//||  |//\\//\\//\\///     |\\||              |\\||  |//\\//\\//\\/||
    |//\\//\\//\\||  |//||  |\\//\\//\\//\||  |//\\//\\//\\||  |\\//\\//\\/\/\      |//||              |//||  |\\//\\//\\//\||
    |\\||            |\\||  |\\||       \/\\  |\\||            |//||      \/\\\     |\\||              |\\||  |\\||       \/\\
    |//||            |//||  |//||       |\||  |//||__________  |\\||       \//\\    |//||____________  |//||  |//||       |\||
    |\\||            |\\||  |\\||_______/\//  |\\//\\//\\//||  |//||        \\/\\   |//\\//\\//\\//||  |\\||  |\\||_______/\//          
    |//||            |//||  |//\\//\\//\\//   |//\\//\\//\\||  |\\||         \\/\\  |\\//\\//\\//\\||  |//||  |//\\//\\//\\//
    ´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´      
    ´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´´                                                                  

    FiberLib
    --------

    Implementação de threads em user-level(fibers) no modelo M para 1(M threads user-level para 1 
    thread kernel-level) com escalonamento preemptivo utilizando o algoritmo round-robin.

    A alocação de memória para ponteiros que guardam e recebem valores de retorno de fibers é de 
    TOTAL RESPONSABILIDADE DOS USUÁRIOS DA BIBLIOTECA. Além disso, as rotinas aqui implementadas
    NÃO EVITAM que recursos possam ser acessados por múltiplas fibers ao mesmo tempo, nem mesmo 
    evitam que o processo se bloqueie devido a joins encadeados.

    by Guilherme Bartasson, Diego Batistuta e Vitor Teixeira, 2019


<h1>Descrição</h1>
  
<p>Este grupo foi responsável por projetar e programar rotinas de criação e gerenciamento de threads em user-space, comumente referidas como “fibers”, que fazem parte de uma biblioteca denominada FiberLib. A FiberLib possui estruturas e rotinas que permitem a criação de threads em user level, seguindo o modelo de M threads no user level mapeadas para uma thread no kernel level (M:1), num escalonamento preemptivo baseado em time-slice, utilizando o algoritmo round-robin. A criação, escalonamento e armazenamento de contexto das threads foi feita utilizando system calls de manipulação de contextos, sendo elas: getcontext(), setcontext(), makecontext() e swapcontext(), além de system calls de manipulação de timers e sinais, sendo elas: setitimer(), getitimer() e sigaction().</p>
<p>Essa biblioteca foi implementada com base em várias bibliotecas com o mesmo intuito(implementação de threads) e nos conhecimentos prévios de estruturas de dados dos integrantes do grupo. Apesar de isso ter exigido bastante esforço e dedicação, o resultado foi eficiente e satisfatório até onde pôde ser analisado, no que se refere aos requisitos do trabalho. A FiberLib foi criada em formato de shared library e pode ser utilizada, de forma amadora, em programas que necessitem de multi-threading simples. Pelo fato de ela não possuir nenhuma estrutura para semáforos e outras formas de evitar bloqueios do processo e acesso de áreas críticas por múltiplas threads, evitar que problemas relacionados a isso aconteçam é de total responsabilidade de seus usuários.</p>   
<p>As rotinas fiber_create(), fiber_exit() e fiber_join()  foram implementadas com base na funcionalidade das rotinas equivalentes da biblioteca pthread, sendo elas, respectivamente: pthread_create(), pthread_exit() e pthread_join(). Há um  arquivo de cabeçalho “fiber.h” que possui os símbolos necessários para o uso das rotinas da biblioteca e um arquivo “fiber.c” com os códigos das rotinas e declarações das estruturas e variáveis globais utilizadas por elas.</p>
<p>Pelo fato de o escalonamento ser baseado no algoritmo round-robin, uma maneira intuitiva e simples de implementá-lo é por meio de uma lista circular, e por isso a FiberLib armazena as fibers em uma lista desse tipo, com sua cauda apontando para a cabeça, assim simplificando o trabalho do escalonador.</p>
