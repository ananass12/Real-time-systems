//бесконечно выводит переданные аргументы командной строки, нумеруя каждую строку и делая паузу в 1 секунду между выводами
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
int main(int argc, char* argv[])
{
  int i = 0, j;
  setvbuf(stdout, NULL, _IOLBF, 0); //буфер сбрасывается при выводе символа новой строки
  if (argc == 1) { //если передано только имя программы
    printf("Программу следует запускать с аргументами командной строки\n\n");
    return EXIT_FAILURE;
  }
  while (1) {
    printf("#%d: ", i++);
    for (j = 1; j < argc; j++) {
      printf("%s ", argv[j]);
    }
    printf("\n");
    sleep(1);
  }
  return EXIT_SUCCESS;
}