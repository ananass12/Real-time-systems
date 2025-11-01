 #include <stdio.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <sched.h>
 
 volatile int        state;   //текущее состояние
 volatile int counter = 0;  //счетчик для определения чётности
 pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER; //мьютекс
 pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;  //условная переменная
 
 //функции обработки состояний
 void *state_0(void *arg);
 void *state_1(void *arg);
 void *state_2(void *arg);
 void *state_3(void *arg);
 char    *progname = "condvar";
 
 int main ()
 {
     setvbuf (stdout, NULL, _IOLBF, 0);

     pthread_t t0, t1, t2, t3;  //идентификаторы потоков
     pthread_create(&t0, NULL, state_0, NULL);
     pthread_create(&t1, NULL, state_1, NULL);
     pthread_create(&t2, NULL, state_2, NULL);
     pthread_create(&t3, NULL, state_3, NULL);

     sleep (20); 
     printf ("%s:  main, exiting\n", progname);
     return 0;
 }
 
 //обработчик состояния 0 - всегда переходит в 1
 void *state_0 (void *arg)
 {
     while (1) {
         pthread_mutex_lock (&mutex);
         while (state != 0) {
             pthread_cond_wait (&cond, &mutex);
         }
         printf ("%s:  transit 0 -> 1\n", progname);
         state = 1;
         counter++; 
         usleep(100 * 1000);
         pthread_cond_signal (&cond);   //уведомляем другие потоки
         pthread_mutex_unlock (&mutex);
     }
     return (NULL);
 }
 
 //обработчик состояния 1 - проверка четности
 void *state_1 (void *arg)
 {
     while (1) {
         pthread_mutex_lock (&mutex);
         while (state != 1) {
            pthread_cond_wait(&cond, &mutex);
         }
         if (counter % 2 == 0) {
            printf("%s: transit 1 -> 2\n", progname);
            state = 2;
         } else {
            printf("%s: transit 1 -> 3\n", progname);
            state = 3;
         }
         pthread_cond_signal (&cond);
         pthread_mutex_unlock (&mutex);
     }
     return (NULL);
 }

//обработчик состояния 2 - переходит в 0
 void *state_2(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (state != 2) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("%s: transit 2 -> 0\n", progname);
        state = 0;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

//обработчик состояния 3 - переходит в 0
void *state_3(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (state != 3) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("%s: transit 3 -> 0\n", progname);
        state = 0;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}