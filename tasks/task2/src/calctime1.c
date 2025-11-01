//Демонстрация работы с системными часами POSIX
#define _POSIX_C_SOURCE 200112L  //макрос чтобы не ругалось на CLOCK_REALTIME

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> // Для clock_gettime

#define BILLION 1000000000LL  //Константа для перевода секунд в наносекунды (1с = 1000000000нс)
#define NumSamples 10  //количество измерений

char *progname = "calctime1";

int main() {
    struct timespec clockval, prevclockval;  //clockval - текущее значение времени, prevclockval - предыдущее значение времени
    uint64_t delta; //разница между ними в наносекундах

    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("\nMeasuring the minimum possible delta for CLOCK_REALTIME:\n");

    for (int i = 0; i < NumSamples; i++) {     //цикл ждет первое изменение показаний часов
        clock_gettime(CLOCK_REALTIME, &prevclockval);
        do {
            clock_gettime(CLOCK_REALTIME, &clockval);
        } while (clockval.tv_sec == prevclockval.tv_sec &&
                 clockval.tv_nsec == prevclockval.tv_nsec);

        delta = ((uint64_t)clockval.tv_sec * BILLION + (uint64_t)clockval.tv_nsec) -
                ((uint64_t)prevclockval.tv_sec * BILLION + (uint64_t)prevclockval.tv_nsec);
        //преобразуем оба времени в абсолютное количество наносекунд с эпохи Unix и вычитаем — получаем delta в нс

        printf("prev %ld.%09ld, new %ld.%09ld, delta %" PRIu64 " ns\n",
               prevclockval.tv_sec, prevclockval.tv_nsec, clockval.tv_sec,
               clockval.tv_nsec, delta);
    }

    printf("\n%s: End of work.\n", progname);
    return EXIT_SUCCESS;
}