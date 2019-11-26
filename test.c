#include <stdio.h>
#include <ucontext.h>
#include "fiber.h"

#define NUM_FIBER 27

fiber_t fibers[NUM_FIBER];
ucontext_t parent, child;
int finish = 0;

void *threadFunction1(void * a) {
    printf("Pelo amor de Deus bota uma coisa na sua cabeça...\n");
    while(1);
}

void *threadFunction2(void * b) {
    printf("Não da pra discutir com os caras!\n");
    while(1);
}

void *threadFunction3(void * c) {
    printf("Não é que eu sou mais vascaino, menos vascaino não meu irmão...\n");
    while(1);
}

void *threadFunction4(void * c) {
    printf("O time dos cara vale 10 vezes mais que o nosso...\n");
    while(1);
}

void *threadFunction5(void * c) {
    printf("E cê ta falando que nois contratou 8?!\n");
    while(1);
}

void *threadFunction6(void * c) {
    printf("contratou 8.... UM, me fala UM que presta!\n");
    while(1);
}

void *threadFunction7(void * c) {
    printf("Porra, Felipe Bastos, uma merda...\n");
    while(1);
}

void *threadFunction8(void * c) {
    printf("Nem sport quer esses cara que nois contratou, ninguém quer porra!\n");
    while(1);
}

void *threadFunction9(void * c) {
    printf("Nois só pega nego que ninguém quer porra!\n");
    while(1);
}

void *threadFunction10(void * c) {
    printf("Bruno César, 102 quilo, porra pesadão...\n");
    while(1);
}

void *threadFunction11(void * c) {
    printf("Cê acha que se ele fosse bom nego ia libera ele pra nois sem paga nada?!\n");
    while(1);
}

void *threadFunction12(void * c) {
    printf("AAAAAAAAH!\n");
    while(1);
}

void *threadFunction13(void * c) {
    printf("Se não nego queria ir pro outro time po... ACORDA!!!\n");
    while(1);
}

void *threadFunction14(void * c) {
    printf("Meu irmão não da pra discutir com flamenguista no momento...\n");
    while(1);
}

void *threadFunction15(void * c) {
    printf("Nois tamo na merda, ah mas os cara nao ganharam nada, ok....\n");
    while(1);
}

void *threadFunction16(void * c) {
    printf("NOIS TAMBÉM NÃO GANHOU!!!!\n");
    while(1);
}

void *threadFunction17(void * c) {
    printf("Os cara ficaram em segundo no brasileiro, NOIS QUASE CAIU, PORRA JEAN CÊ TA MULUCO?!!\n");
    while(1);
}

void *threadFunction18(void * c) {
    printf("VAI DISCUTIR PRA QUE?!! PRA SER ZUADO?!! CALA A BOCA E FICA QUIETO!!!\n");
    while(1);
}

void *threadFunction19(void * c) {
    printf("CARA OS CARA CONTRATO 1!!! FODA-SE!! NOIS CONTRATOU 8, NENHUM PRESTO MEU IRMÃO!!!\n");
    while(1);
}

void *threadFunction20(void * c) {
    printf("É MELHOR OS CARA NÃO GANHOU NADA, MAS OS CARA SÃO FELIZ, TEM DINHEIRO!!!\n");
    while(1);
}

void *threadFunction21(void * c) {
    printf("NOIS NÃO GANHO NADA E SOMO POBRE!!!\n");
    while(1);
}

void *threadFunction22(void * c) {
    printf("ME, RESPONDE UMA COISA...\n");
    while(1);
}

void *threadFunction23(void * c) {
    printf("VOCÊ PREFERE SER POBRE E TRISTE?!!!\n");
    while(1);
}

void *threadFunction24(void * c) {
    printf("OU TRISTE E RICO?!!! VIAJA?!!! Esquece a pobreza?\n");
    while(1);
}

void *threadFunction25(void * c) {
    printf("Os cara ai, gabigol ai, a felicidade dos cara, os cara contrataram gabi gol...\n");
    while(1);
}

void *threadFunction26(void * c) {
    printf("Meteu 3 na gente dentro do maracanâ...\n");
    while(1);
}

void *threadFunction27(void * c) {
    printf("PORRA!!! ACORDA CARALHO!!!\n\n");
    while(1);
}

int main () {
    getcontext(&parent);

    if (finish == 0) {
        void * arg = NULL;

        fiber_create(&fibers[0], threadFunction1, arg);
        fiber_create(&fibers[1], threadFunction2, arg);
        fiber_create(&fibers[2], threadFunction3, arg);
        fiber_create(&fibers[3], threadFunction4, arg);
        fiber_create(&fibers[4], threadFunction5, arg);
        fiber_create(&fibers[5], threadFunction6, arg);
        fiber_create(&fibers[6], threadFunction7, arg);
        fiber_create(&fibers[7], threadFunction8, arg);
        fiber_create(&fibers[8], threadFunction9, arg);
        fiber_create(&fibers[9], threadFunction10, arg);
        fiber_create(&fibers[10], threadFunction11, arg);
        fiber_create(&fibers[11], threadFunction12, arg);
        fiber_create(&fibers[12], threadFunction13, arg);
        fiber_create(&fibers[13], threadFunction14, arg);
        fiber_create(&fibers[14], threadFunction15, arg);
        fiber_create(&fibers[15], threadFunction16, arg);
        fiber_create(&fibers[16], threadFunction17, arg);
        fiber_create(&fibers[17], threadFunction18, arg);
        fiber_create(&fibers[18], threadFunction19, arg);
        fiber_create(&fibers[19], threadFunction20, arg);
        fiber_create(&fibers[20], threadFunction21, arg);
        fiber_create(&fibers[21], threadFunction22, arg);
        fiber_create(&fibers[22], threadFunction23, arg);
        fiber_create(&fibers[23], threadFunction24, arg);
        fiber_create(&fibers[24], threadFunction25, arg);
        fiber_create(&fibers[25], threadFunction26, arg);
        fiber_create(&fibers[26], threadFunction27, arg);

        //fiberSwap(&fibers[0]);

        startFibers();
    } else {
        getnFibers();
        printf("Processo completo\n");
    }

    return 0;
}