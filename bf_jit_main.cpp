#include <stdio.h>
#include <stdlib.h>

#include "bf_jit.h"

// g++ -o bf_jit64 -m64 bf_jit_main.cpp bf_jit64.cpp
// g++ -o bf_jit32 -m32 bf_jit_main.cpp bf_jit32.cpp

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("You need to specify a single file.", stderr);
  }

  FILE *bf_source_file = fopen(argv[1], "rb");
  if (bf_source_file == NULL) {
    fputs("Could not open file.", stderr);
    return 1;
  }

  fseek(bf_source_file, 0, SEEK_END);
  size_t source_size = ftell(bf_source_file);
  rewind(bf_source_file);

  char* source = (char *) malloc(source_size);
  size_t amount_read = fread(source, 1, source_size, bf_source_file);
  fclose(bf_source_file);
  if (amount_read != source_size) {
    fputs("Problem reading file.", stderr);
    return 1;
  }

  BrainfuckProgram bf;
  void* memory = calloc(50 * 1024, 1);

  if (!bf.init(string(source, source_size))) {
    return 1;
  }

  bf.run(memory);
  return 0;
}
