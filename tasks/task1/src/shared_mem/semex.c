//работа POSIX семафора как средства синхронизации между потоками

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

sem_t   *mySemaphore; //глобальный указатель на семафор общий для всех потоков
//функций потоков
void    *producer (void *);
void    *consumer (void *);
char    *progname = "semex";

#define SEM_NAME "/Semex" //имя семафора
#define Named 1  //флаг именованный семафор 1 или неименованный 0

int main ()
{
    int     i;
    setvbuf (stdout, NULL, _IOLBF, 0); 
#ifdef  Named  //именованный семафор
    //O_CREAT — создаем, если не существует
    //S_IRWXU — права доступа - владелец может читать, писать, выполнять
    //0 — начальное значение счётчика семафора (никто не может пройти)
    mySemaphore = sem_open (SEM_NAME, O_CREAT, S_IRWXU, 0);
    //удаляем имя - не используется совместно с другими процессами
    sem_unlink( SEM_NAME );
#else   //неименованный семафор
    mySemaphore = malloc (sizeof (sem_t));
    sem_init (mySemaphore, 0, 0);
#endif 

    pthread_t consumers[5];  //массив для хранения ID 5 потоков потребителей
    pthread_t producerThread;

    for (i = 0; i < 5; i++) {  //создаем 5 потоков потребителей
        pthread_create (&consumers[i], NULL, consumer, (void *)(long)i);
    }
    pthread_create (&producerThread, NULL, producer, (void *) 1);  //создаем 1 поток производитель
    sleep (20);    
    printf ("%s:  main, exiting\n", progname);
    return 0;
}

//поток производитель каждую секунду "освобождает" семафор
void *producer (void *i)
{
    while (1) {
        sleep (1);
        printf ("%s:  (producer %ld), posted semaphore\n", progname, (long)i);
        //увеличиваем счётчик семафора на 1 - разблокируем один ожидающий поток
        sem_post (mySemaphore);
    }
    return (NULL);
}

//поток потребитель ждёт пока семафор не станет доступен
void *consumer (void *i)
{
    while (1) {
        //когда счётчик > 0 — уменьшаем его на 1 и продолжаем выполнение
        sem_wait (mySemaphore);
        printf ("%s:  (consumer %ld) got semaphore\n", progname, (long)i);
        //после вывода зацикливается и ждёт следующего сигнала
    }
    return (NULL);
}