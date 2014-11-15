// Copyright 2014 Brian Quinlan
// See "LICENSE" file for details.
//
// Executes Brainfuck code stored in a file. Flags control the execution mode.
// See http://en.wikipedia.org/wiki/Brainfuck.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <string>
#include <vector>

#include "bf_runner.h"
#include "bf_compile_and_go.h"
#include "bf_interpreter.h"
#include "bf_jit.h"

using std::string;
using std::unique_ptr;
using std::vector;

const size_t kBrainfuckMemorySize = 1024 * 1024;

const char USAGE[] = "Usage: %s [options] <Brainfuck file>\n"
                     "Execute the Brainfuck code in the given file e.g.\n"
                     "%s examples/hello.b\n"
                     "\n"
                     "Options:\n"
                     "--mode=cag : Run using a compiler\n"
                     "--mode=i   : Run using an interpreter\n"
                     "--mode=jit : Run using a Just-In-Time compiler\n";

// Passed to BrainfuckRunner->run(...) to provide output functionality for
// the "." opcode.
static bool bf_write(void*, char c) {
  return putchar(c) != EOF;
}

// Passed to BrainfuckRunner->run(...) to provide input functionality for
// the "," opcode.
static char bf_read(void*) {
  int c = getchar();
  if (c == EOF) {
    return 0;
  } else {
    return c;
  }
}

int run_brainfuck_program(BrainfuckRunner* runner,
                          const string& source_file_path) {
  FILE *bf_source_file = fopen(source_file_path.c_str(), "rb");
  if (bf_source_file == NULL) {
    fprintf(stderr, "Could not open file \"%s\": %s\n",
            source_file_path.c_str(), strerror(errno));
    return 1;
  }

  if (fseek(bf_source_file, 0, SEEK_END)) {
    fprintf(stderr, "Could not seek in \"%s\": %s\n",
            source_file_path.c_str(), strerror(ferror(bf_source_file)));
    return 1;
  }

  long source_size = ftell(bf_source_file);
  if (source_size == -1) {
    fprintf(stderr, "Could not tell in \"%s\": %s\n",
            source_file_path.c_str(), strerror(errno));
    return 1;
  }
  rewind(bf_source_file);

  char* source_buffer = reinterpret_cast<char *>(malloc(source_size));
  size_t amount_read = fread(source_buffer, 1, source_size, bf_source_file);
  fclose(bf_source_file);
  if (amount_read != static_cast<size_t>(source_size)) {
    fprintf(stderr, "Error reading file \"%s\": %s\n",
            source_file_path.c_str(), strerror(errno));
    return 1;
  }

  void* memory = calloc(kBrainfuckMemorySize, 1);
  if (memory == NULL) {
    fprintf(stderr,
            "Unable to allocate memory %ld bytes for Brainfuck memory\n",
            static_cast<unsigned long>(kBrainfuckMemorySize));
  }

  const string source(source_buffer, source_size);

  if (!runner->init(source.begin(), source.end())) {
    return 1;
  }

  runner->run(bf_read, NULL, bf_write, NULL, memory);
  return 0;
}

int main(int argc, char *argv[]) {
  unique_ptr<BrainfuckRunner> bf(new BrainfuckInterpreter());
  string bf_file;

  for (int i = 1; i < argc; ++i) {
    if (argv[i] == string("-h") ||
        argv[i] == string("-help") ||
        argv[i] == string("--help") ||
        argv[i] == string("-?")) {
      printf(USAGE, argv[0], argv[0]);
      return 0;
    }
  }

  vector<string> files;
  for (int i = 1; i < argc; ++i) {
    string arg(argv[i]);
    if (arg.find("--") == 0) {
      if (arg.find("--mode=") == 0) {
        if (arg == "--mode=cag") {
          unique_ptr<BrainfuckRunner> compile_and_go(
              new BrainfuckCompileAndGo());
          bf = std::move(compile_and_go);
        } else if (arg == "--mode=i") {
          unique_ptr<BrainfuckRunner> interpreter(new BrainfuckInterpreter());
          bf = std::move(interpreter);
        } else if (arg == "--mode=jit") {
          unique_ptr<BrainfuckRunner> jit(new BrainfuckJIT());
          bf = std::move(jit);
        } else {
          fprintf(stderr, "Unexpected mode: %s\n", arg.c_str());
          return 1;
        }
      } else {
        fprintf(stderr, "Unexpected argument: %s\n", arg.c_str());
        return 1;
      }
    } else {
      files.push_back(arg);
    }
  }

  if (files.size() != 1) {
    fputs("You need to specify exactly one Brainfuck file\n", stderr);
    printf(USAGE, argv[0], argv[0]);
    return 1;
  }

  return run_brainfuck_program(bf.get(), files[0]);
}
