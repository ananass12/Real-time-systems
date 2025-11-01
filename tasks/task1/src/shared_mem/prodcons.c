/*
 *  Демонстрация POSIX условных переменных на примере "Производитель и потребитель".
 *  Так как у нас всего два потока, ожидающих сигнала,    
 *  в любой момент работы одного из них мы можем просто использовать вызов
 *  pthread_cond_signal для пробуждения второго потока.
 *
*/

#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER; //мьютекс для защиты общих данных
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;  //условная переменная для синхронизации потоков

volatile int        state = 0;    //0 - буфер пуст, 1 - буфер полон
volatile int        product = 0;  //вывод производителя

//функции потоков и вспомогательные функции
void    *producer (void *);
void    *consumer (void *);
void    do_producer_work (void); //имитация работы производителя
void    do_consumer_work (void); //имитация работы потребителя

char    *progname = "prodcons";

int main ()
{
  setvbuf (stdout, NULL, _IOLBF, 0);  //немедленный вывод без буферизации

  pthread_t cons_thread, prod_thread;  //объявляем переменные
  
  //создаем потоки потребитель и производитель
  pthread_create(&cons_thread, NULL, consumer, NULL);
  pthread_create(&prod_thread, NULL, producer, NULL);

  sleep (20);     //работаем 20 секунд
  printf ("%s:  main, exiting\n", progname);
  return 0;
}

//производитель
void *producer (void *arg)
{
  while (1) {
    //блокируем мьютекс перед работой с общими данными
    pthread_mutex_lock (&mutex);

    //ждем пока буфер не освободится (state == 0)
    //while  — защита от ложных пробуждений
    while (state == 1) {
      //освобождает мьютекс и ждёт сигнала, при пробуждении автоматически блокирует мьютекс снова
      pthread_cond_wait (&cond, &mutex);
    }

    //буфер пуст - можно производить
    product++;  //создаём новый продукт
    printf ("%s:  produced %d, state %d\n", progname, ++product, state);
    state = 1;
    pthread_cond_signal (&cond); //уведомляем потребителя, что появился новый продукт
    pthread_mutex_unlock (&mutex);  //разблокируем мьютекс
    do_producer_work ();  //иммитация работы
  }
  return (NULL);
}

//потребитель
void *consumer (void *arg)
{
  while (1) {  //ждем пока буфер не заполнится (state == 1)
    pthread_mutex_lock (&mutex);
    while (state == 0) {
      pthread_cond_wait (&cond, &mutex);
    }

    //буфер полон - можно потреблять
    printf ("%s:  consumed %d, state %d\n", progname, product, state);
    state = 0;
    pthread_cond_signal (&cond); //уведомляем производителя что буфер освободился
    pthread_mutex_unlock (&mutex);
    do_consumer_work ();  //иммитация работы
  }
  return (NULL);
}

//имитация работы - задержка 100 мс
void do_producer_work (void)
{
  usleep (100 * 1000);
}

void do_consumer_work (void)
{
  usleep (100 * 1000);
}