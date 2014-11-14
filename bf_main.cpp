#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <memory>

#include "bf_runner.h"
#include "bf_compile_and_go.h"
#include "bf_interpreter.h"
#include "bf_jit.h"

using std::string;
using std::unique_ptr;

int main(int argc, char *argv[]) {
  unique_ptr<BrainfuckRunner> bf(new BrainfuckInterpreter());
  string bf_file;

  if (argc == 3) {
    string mode(argv[1]);
    if (mode == "--mode=cag") {
      unique_ptr<BrainfuckRunner> compile_and_go(new BrainfuckCompileAndGo());
      bf = std::move(compile_and_go); 
    } else if (mode == "--mode=i") {
      unique_ptr<BrainfuckRunner> interpreter(new BrainfuckInterpreter());
      bf = std::move(interpreter); 
    } else if (mode == "--mode=jit") {
      unique_ptr<BrainfuckRunner> jit(new BrainfuckJIT());
      bf = std::move(jit);
    } else {
      fprintf(stderr, "Unexpected mode: %s\n", argv[1]);
      return 1;
    }
    bf_file = argv[2];
  } else if (argc == 2) {
    bf_file = argv[1];    
  } else {
    fputs("You need to specify a single file.", stderr);
    return 1;    
  }

  FILE *bf_source_file = fopen(bf_file.c_str(), "rb");
  if (bf_source_file == NULL) {
    fputs("Could not open file.", stderr);
    return 1;
  }

  fseek(bf_source_file, 0, SEEK_END);
  size_t source_size = ftell(bf_source_file);
  rewind(bf_source_file);

  char* source_buffer = (char *) malloc(source_size);
  size_t amount_read = fread(source_buffer, 1, source_size, bf_source_file);
  fclose(bf_source_file);
  if (amount_read != source_size) {
    fputs("Problem reading file.", stderr);
    return 1;
  }

  void* memory = calloc(50 * 1024, 1);
  const string source(source_buffer, source_size);

  if (!bf->init(source.begin(), source.end())) {
    return 1;
  }

  bf->run(memory);
  return 0;
}
