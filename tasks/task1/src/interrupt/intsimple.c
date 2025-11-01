#define _POSIX_C_SOURCE 200809L  //макрос определения функциональности POSIX

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static const char *progname = "intsimple";

//Флаги, устанавливаемые обработчиками сигналов
static volatile sig_atomic_t got_sigint  = 0;
static volatile sig_atomic_t got_sigterm = 0;
static volatile sig_atomic_t got_sigusr1 = 0;
static volatile sig_atomic_t got_sigusr2 = 0;

//сохраняем исходные настройки терминала, чтобы восстановить их при выходе
static struct termios orig_termios;

//восстанавливаем исходный режим терминала при завершении программы
static void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

static int enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        return -1;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);  //отключаем канонический режим и эхо
    raw.c_cc[VMIN]  = 0;  //минимум символов для read() = 0 - неблокирующий
    raw.c_cc[VTIME] = 0;  //таймаут 0 - сразу возвращает, даже если нет данных

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
        return -1;

    //функция восстановления терминала при выходе
    atexit(restore_terminal);
    return 0;
}

//обработчики сигналов устанавливают флаги
static void handle_sigint(int signo)  { (void)signo; got_sigint  = 1; }
static void handle_sigterm(int signo) { (void)signo; got_sigterm = 1; }
static void handle_sigusr1(int signo) { (void)signo; got_sigusr1 = 1; }
static void handle_sigusr2(int signo) { (void)signo; got_sigusr2 = 1; }

int main(void) {
    //построчный вывод для stdout - сразу видим сообщения
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("%s: starting...\n", progname);
    printf("Поддерживаемые сигналы: SIGINT(Ctrl+C), SIGTERM, SIGUSR1, SIGUSR2.\n");
    printf("Замечание: SIGKILL нельзя перехватить или обработать на Linux.\n");
    printf("Нажмите 'q' для выхода.\n");

    //raw-режим терминала
    if (enable_raw_mode() == -1) {
        perror("termios");
        return EXIT_FAILURE;
    }

    //обработчики сигналов
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = handle_sigusr1;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigusr2;
    sigaction(SIGUSR2, &sa, NULL);

    for (;;) {
        //проверяем сигналы и выводим сообщения
        if (got_sigint)  { got_sigint  = 0; printf("%s: получен SIGINT (Ctrl+C)\n", progname); }
        if (got_sigterm) { got_sigterm = 0; printf("%s: получен SIGTERM\n", progname); }
        if (got_sigusr1) { got_sigusr1 = 0; printf("%s: получен SIGUSR1\n", progname); }
        if (got_sigusr2) { got_sigusr2 = 0; printf("%s: получен SIGUSR2\n", progname); }

        //неблокирующее чтение одного символа с клавиатуры
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);

        if (n == 1) {
            if (ch == 'q' || ch == 'Q') {
                printf("%s: выход по клавише 'q'\n", progname);
                break;
            }
            //игнорируем символы перевода строки
            if (ch != '\n' && ch != '\r') {
                printf("%s: клавиша '%c'\n", progname, ch);
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            //ошибка несвязанная с отсутствием данных
            perror("read");
            break;
        }
        sleep(10 * 1000);  // 10 мс
    }

    printf("%s: exiting...\n", progname);
    return EXIT_SUCCESS;
}