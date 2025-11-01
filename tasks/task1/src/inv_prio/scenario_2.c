#include "working.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

//настройка приоритета потока
static int set_thread_priority(pthread_attr_t *attr, int policy, int prio)
{
    pthread_attr_init(attr);
    pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(attr, policy);
    
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = prio;
    return pthread_attr_setschedparam(attr, &sp);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    //приоритеты как в scenario_1
    const int policy = SCHED_FIFO;
    const int prio_server = 10;  //низкий
    const int prio_t1     = 20;  //средний
    const int prio_t2     = 30;  //высокий

    //включаем наследование приоритета (PTHREAD_PRIO_INHERIT)
    if (init_resource_mutex(1) != 0) {
        perror("init_resource_mutex (with priority inheritance)");
        return EXIT_FAILURE;
    }

    //атрибуты для трёх потоков
    pthread_attr_t attr_server, attr_t1, attr_t2;
    set_thread_priority(&attr_server, policy, prio_server);
    set_thread_priority(&attr_t1,     policy, prio_t1);
    set_thread_priority(&attr_t2,     policy, prio_t2);

    pthread_t th_server, th_t1, th_t2;   //идентификаторы потоков

    //запускаем низкоприоритетный сервер — он захватит мьютекс
    if (pthread_create(&th_server, &attr_server, server, NULL) != 0) {
        perror("pthread_create server");
        return EXIT_FAILURE;
    }
    usleep(200 * 1000); //даем фору

    //запускаем высокий приоритет 
    if (pthread_create(&th_t2, &attr_t2, t2, NULL) != 0) {
        perror("pthread_create t2");
        return EXIT_FAILURE;
    }
    //запускаем среднеприоритетный поток
    if (pthread_create(&th_t1, &attr_t1, t1, NULL) != 0) {
        perror("pthread_create t1");
        return EXIT_FAILURE;
    }

    //ждем завершения
    void *st;
    pthread_join(th_t1, &st);
    pthread_join(th_t2, &st);
    pthread_join(th_server, &st);

    printf("scenario_2 завершено\n");
    return EXIT_SUCCESS;
}