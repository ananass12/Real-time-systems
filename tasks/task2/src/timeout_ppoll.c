/*
 Демонстрация ppoll() для ожидания с атомарной разблокировкой сигналов.

 Цель: Показать, как правильно и безопасно обрабатывать сигналы во время блокирующего ожидания на файловых дескрипторах. Простое использование
 poll() или read() может привести к состоянию гонки (race condition), если сигнал придет до того, как блокирующий вызов начался.
 ppoll() решает эту проблему, атомарно изменяя маску сигналов на время ожидания.
 
 Сценарий:
 1. Основной поток блокирует сигнал SIGUSR1 с помощью pthread_sigmask().
 2. Создается дочерний поток, который через 1 секунду посылает SIGUSR1 основному.
 3. Основной поток вызывает ppoll(), передавая ему маску сигналов, в которой SIGUSR1 РАЗБЛОКИРОВАН.
 4. ppoll() атомарно снимает блокировку с SIGUSR1 и начинает ждать.
 5. Когда приходит сигнал, он прерывает ppoll() (возвращается EINTR), и обработчик сигнала выполняется немедленно.
 6. После возврата из ppoll() исходная маска (с заблокированным SIGUSR1) автоматически восстанавливается.
*/

/*
Сравнение четырех механизмов обработки сигналов и ожидания:

1. signal() + read()/poll():
- Возможна race condition - сигнал может прийти до вызова read()/poll() и будет потерян, что приведет к вечному блокированию
- API: signal(signum, handler); read(fd, buf, size);
- Использование - простые приложения, где потеря сигнала не критична.
2. sigaction() + read()/poll() с SA_RESTART:
- Сигнал прерывает системный вызов, но SA_RESTART автоматически перезапускает его. Однако не все системные вызовы поддерживают перезапуск.
- API: sigaction() с флагом SA_RESTART; read()/poll();
- Использование - когда нужно автоматическое восстановление после сигнала.
3. sigprocmask() + sigwait() + poll():
- РЕШЕНИЕ: Сигналы блокируются и обрабатываются синхронно через sigwait() в отдельном цикле или потоке.
- API: sigprocmask() -> poll() -> sigwait() в цикле
- Использование - серверы, которые должны обрабатывать сигналы и события от сокетов детерминированным образом.
4. sigprocmask() + ppoll()/pselect():
- РЕШЕНИЕ: Атомарное разблокирование сигналов на время ожидания, что исключает race condition. 
Сигнал либо приходит ДО начала ожидания (и обрабатывается), либо ВО ВРЕМЯ ожидания (прерывает его).
- API: sigprocmask() -> ppoll() с передачей маски разблокированных сигналов
- Использование - критические приложения, где нельзя потерять ни один сигнал и нужна максимальная надежность.
 */


#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Флаг, который будет (безопасно) установлен в обработчике сигнала.
// volatile - чтобы компилятор не оптимизировал доступ к переменной.
// sig_atomic_t - тип, запись в который является атомарной.
static volatile sig_atomic_t signal_received = 0;
static volatile sig_atomic_t handler_executed = 0;

static void signal_handler(int signum) {
    (void)signum;
    signal_received = 1;
    handler_executed = 1;
    //write() - одна из немногих async-signal-safe функций.
    //Использовать printf() и другие функции стандартной библиотеки в обработчиках сигналов НЕБЕЗОПАСНО и может привести к deadlock.
    write(STDOUT_FILENO, "Signal handler executed!\n", 25);
}


// Поток, посылающий сигнал
static void *thread_sender(void *arg) {
    (void)arg;
    printf("[SENDER] sleeping for 1 second...\n");
    sleep(1);
    printf("[SENDER] sending SIGUSR1 to main thread...\n");
    kill(getpid(), SIGUSR1);   //pthread_main_np() не работало
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    //1.Настройка обработчика сигнала
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    //2.Блокируем SIGUSR1 с помощью sigprocmask
    sigset_t blocked_mask, original_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask) != 0) {
        perror("pthread_sigmask");
        return EXIT_FAILURE;
    }
    printf("Main thread blocked SIGUSR1.\n");

    //3.Создаем pipe, на котором будем ждать (хотя данных там не будет)
    int fds[2];
    if (pipe(fds) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    struct pollfd pfd = {.fd = fds[0], .events = POLLIN};

    // 4. Создаем поток, который пошлет нам сигнал
    pthread_t tid;
    if (pthread_create(&tid, NULL, thread_sender, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    // 5. Вызываем ppoll().
    // Он будет ждать 5 секунд И атомарно заменит текущую маску сигналов (где SIGUSR1 заблокирован) на original_mask (где он разблокирован).
    // Как только ppoll вернет управление, исходная маска будет восстановлена.
    printf("Calling ppoll() with unblocked signal mask, waiting for signal...\n");
    struct timespec timeout = {.tv_sec = 5, .tv_nsec = 0};

    // Это ключевой вызов. Передача `&original_mask` - это то, что отличает ppoll от poll и решает проблему race condition.
    int rc = ppoll(&pfd, 1, &timeout, &original_mask);

    // 6. Анализ результата
    if (rc == -1) {
        if (errno == EINTR) {
            printf("ppoll() был прерван сигналом (EINTR)\n");
        } else {
            perror("ppoll завершился с ошибкой");
        }
    } else if (rc == 0) {
        printf("ppoll() завершился по таймауту (сигнал не получен)\n");
    } else {
        printf("ppoll() обнаружил данные в pipe\n");
    }

    if (handler_executed) {
        printf("Обработчик сигнала был выполнен\n");
    } else {
        printf("Обработчик сигнала не был выполнен\n");
    }

    if (signal_received) {
        printf("Флаг signal_received установлен\n");
    } else {
        printf("Флаг signal_received не установлен\n");
    }

    pthread_join(tid, NULL);
    close(fds[0]);
    close(fds[1]);
    return 0;
}
