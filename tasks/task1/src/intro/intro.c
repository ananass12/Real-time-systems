#include <stdlib.h> //EXIT_SUCCESS
#include <stdio.h>  //printf, perror
#include <pthread.h> //потоки
#include <sched.h>   //приоритеты потоков
#include <unistd.h>  //usleep
#include <signal.h>   //обработка Ctrl+C

 
void *sense(void* arg);  //читает символы от пользователя и обновляет текущее состояние системы, если символ допустим
void *stateOutput(void* arg);  //реагирует на изменения состояния
void *userInterface(void* arg); //выводит интерфейс состояния 

short isRealState(char s);  //проверка на допустимость символа
 
char state; //N, R, D в любом регистре
short changed; //флаг изменения состояния

pthread_cond_t stateCond;  //условная переменная для уведомления о событии
pthread_mutex_t stateMutex;  //мьютекс для защиты доступа к state и changed
 
#define TRUE 1 
#define FALSE 0 
 
int main(int argc, char *argv[]) { 
 
    printf("Hello World!\n"); 
 
    state = 'N';  //начальное состояние Not Ready
    //инициализация
    pthread_cond_init(&stateCond, NULL); 
    pthread_mutex_init(&stateMutex, NULL); 
 
    //идентификаторы потоков
    pthread_t sensorThread;   //ввод
    pthread_t stateOutputThread;  //вывод изменений
    pthread_t userThread;    //интерфейс
 
    //создание и запуск потоков
    pthread_create(&sensorThread, NULL, sense, NULL); 
    pthread_create(&stateOutputThread, NULL, stateOutput, NULL); 
    pthread_create(&userThread, NULL, userInterface, NULL); 
 
    //ждем завершения потоков
    pthread_join(sensorThread, NULL); 
    pthread_join(stateOutputThread, NULL); 
    pthread_join(userThread, NULL); 
 
    printf("Exit Success!\n"); 
    return EXIT_SUCCESS; 
} 
 
// Поток sense определяет, изменилось ли состояние - сигнальный поток.
void *sense(void* arg) { 
    char prevState = ' '; //запоминаем предыдущее состояние

    while (TRUE) { 
    char tempState; //временная переменная
    scanf("%c", &tempState);   //считываем 1 символ
    usleep(10 * 1000);
 
    pthread_mutex_lock(&stateMutex);       //блокируем мьютекс перед модификацией (state, changed)
 
    if (isRealState(tempState)) {  //если символ допустим обновляем глобальное состояние
    state = tempState;
    }
 
    //если состояние изменилось уведомляем поток stateOutput
    //state ^ ' ' инвертирует регистр символа: x -> X
    if (prevState != state && prevState != (state ^ ' ')) { 
    changed = TRUE;     //флаг для ожидающего потока
    pthread_cond_signal(&stateCond);   //сигнализируем ожидающему потоку stateOutput
    } 
 
    prevState = state;  //обновляем предыдущее состояние
 
    //разблокируем мьютекс — другие потоки могут теперь читать и писать
    pthread_mutex_unlock(&stateMutex); 
    } 
    return NULL; 
} 
 
//вспомогательная функция проверки допустимого состояния
short isRealState(char s) { 
    short real = FALSE; 
 
    if (s == 'R' || s == 'r') //Ready 
    real = TRUE; 
    else if (s == 'N' || s == 'n') //Not Ready 
    real = TRUE; 
    else if (s == 'D' || s == 'd') //Run Mode 
    real = TRUE; 
 
    return real; 
} 
 
//поток stateOutput выводит изменение состояния — ожидающий поток.
void *stateOutput(void* arg) { 
changed = FALSE; 
while (TRUE) { 
    pthread_mutex_lock(&stateMutex);      //блокируем мьютекс перед ожиданием
 
    //используем while (а не if) как защиту от ложных пробуждений
    while (!changed) {     //пока changed не станет TRUE
    //освобождаем мьютекс и ждем сигнал, когда сигнал приходит блокируем мьютекс снова
    pthread_cond_wait(&stateCond, &stateMutex); 
    } 
 
    //вывод нового состояния
    printf("The state has changed! It is now in "); 
    if (state == 'n' || state == 'N') //Not ready 
    printf("Not Ready State\n"); 
    else if (state == 'r' || state == 'R') //Ready 
    printf("Ready State\n"); 
    else if (state == 'd' || state == 'D') //Run Mode 
    printf("Run Mode\n"); 
 
    changed = FALSE;    //сбрасываем флаг изменения
 
    pthread_mutex_unlock(&stateMutex);  //разблокировка мьютекса
    } 
    return NULL; 
} 
 
//поток userInterface выводит визуализацию состояния
void *userInterface(void* arg) { 
while (TRUE) { 
    if (state == 'n' || state == 'N') //Not ready 
    printf("___________________________________________________\n"); 
    else if (state == 'r' || state == 'R') //Ready 
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); 
    else if (state == 'd' || state == 'D') //Run Mode 
    printf("\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/\n"); 
    usleep(1000 * 1000);
    } 
    return NULL; 
}