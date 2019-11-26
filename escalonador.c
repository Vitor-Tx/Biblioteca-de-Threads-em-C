#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

void f (int signum) {
    int static count = 1;
    printf("CARALHO!!!! COUNTER: %d\n", count++);
}

int main () {
    struct sigaction sa;
    struct itimerval timer;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &f;
    sigaction (SIGVTALRM, &sa, NULL);
 
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 500000;
 
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500000;

    setitimer (ITIMER_VIRTUAL, &timer, NULL);

    while (1);
}