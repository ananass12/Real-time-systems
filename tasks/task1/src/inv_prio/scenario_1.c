#include "working.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>

//вспомогательная функция настраивает атрибуты потока для заданной политики планирования и приоритета
static int set_thread_priority(pthread_attr_t *attr, int policy, int prio)
{
  pthread_attr_init(attr); //инициализируем атрибуты потока
  pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);   //поток не наследует параметры планирования от родителя
  pthread_attr_setschedpolicy(attr, policy);  //устанавливаем политику планирования
  
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));  //обнуляем структуру
  sp.sched_priority = prio;   //устанавливаем приоритет
  return pthread_attr_setschedparam(attr, &sp);  //применяем параметры
}

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  //чем выше число — тем выше приоритет
  const int policy = SCHED_FIFO;
  const int prio_server = 10;
  const int prio_t1 = 20;
  const int prio_t2 = 30;

  //мьютекс без наследования приоритета - демонстрация инверсии
  if (init_resource_mutex(0) != 0) {
    perror("init_resource_mutex");
    return EXIT_FAILURE;
  }

  //атрибуты для трёх потоков
  pthread_attr_t attr_server, attr_t1, attr_t2;
  //настраиваем приоритеты для каждого потока
  if (set_thread_priority(&attr_server, policy, prio_server) != 0) {
    perror("attr_server");
  }
  if (set_thread_priority(&attr_t1, policy, prio_t1) != 0) {
    perror("attr_t1");
  }
  if (set_thread_priority(&attr_t2, policy, prio_t2) != 0) {
    perror("attr_t2");
  }

  pthread_t th_server, th_t1, th_t2;  //идентификаторы потоков

  // Запускаем сервер (низкий приоритет) — он захватит ресурс и будет держать
  if (pthread_create(&th_server, &attr_server, server, NULL) != 0) {
    perror("pthread_create server");
    return EXIT_FAILURE;
  }
  //небольшая фора серверу чтобы он успел захватить мьютекс
  usleep(1);

  //запускаем высокий приоритет 
  //он попытается захватить тот же мьютекс - заблокируется и будет ждать
  if (pthread_create(&th_t2, &attr_t2, t2, NULL) != 0) {
    perror("pthread_create t2");
    return EXIT_FAILURE;
  }

  //запускаем среднеприоритетный поток (t1), он не использует мьютекс, но активно грузит CPU.
  //у него приоритет выше, чем у server (20 > 10)
  //он будет забирать процессорное время у server
  if (pthread_create(&th_t1, &attr_t1, t1, NULL) != 0) {
    perror("pthread_create t1");
    return EXIT_FAILURE;
  }

  //ожидание завершения всех потоков
  void *st;
  pthread_join(th_t1, &st);  // Ждём завершения фонового потока
  pthread_join(th_t2, &st);  // Ждём высокоприоритетный (он разблокируется только когда server освободит мьютекс)
  pthread_join(th_server, &st);  // Ждём низкоприоритетный

  printf("scenario_1: завершено. Наблюдайте временную задержку у t2 из-за t1 (инверсия приоритетов).\n");
  return EXIT_SUCCESS;

}