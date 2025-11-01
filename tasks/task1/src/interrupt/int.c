#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

static volatile sig_atomic_t counter = 0;

void on_alarm(int sig) {
    (void)sig;
    if (++counter % 100 == 0) {
        const char s[] = "100 events\n";
        ssize_t ret = write(STDOUT_FILENO, s, sizeof(s)-1);
        (void)ret;
    }
}

int main(void) {
    struct sigaction sa = {0};
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval t = {{0, 10000}, {0, 10000}};
    setitimer(ITIMER_REAL, &t, NULL);

    //ожидание 20 секунд
    time_t start = time(NULL);
    while (time(NULL) - start < 20) {
        sleep(1);
    }

    t.it_value.tv_sec = t.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &t, NULL);

    return 0;
}