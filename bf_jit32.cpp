#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include <string>


#include "bf_jit.h"

const char PREFIX[] =
// enter:
    "\x55"                      // push   %ebp
    "\x89\xe5"                  // mov    %esp,%ebp
    "\x83\xec\x0c"              // sub    $0xc,%esp
    "\x89\x5c\x24\x08"          // mov    %ebx,0x8(%esp)
    "\x8b\x5d\x18"              // mov    0x18(%ebp),%ebx
    "\xeb\x09"                  // jmp    run
// exit:
    "\x8b\x5c\x24\x08"          // mov    0x8(%esp),%ebx
    "\x83\xc4\x0c"              // add    $0xc,%esp
    "\xc9"                      // leave  
    "\xc3";                     // ret

const int EXIT_OFFSET = 15;

const char LEFT[] =
    "\x83\xeb\x01";             // sub    $0x1,%ebx

const char RIGHT[] =
    "\x83\xc3\x01";             // sub    $0x1,%ebx

const char SUBTRACT[] =
    "\x8a\x03"                  // mov    (%ebx),%al
    "\x2c\x01"                  // sub    $0x1,%al
    "\x88\x03";                 // mov    %al,(%ebx)

const char ADD[] =
    "\x8a\x03"                  // mov    (%ebx),%al
    "\x04\x01"                  // add    $0x1,%al
    "\x88\x03";                 // mov    %al,(%ebx)

const char READ[] =
    "\x8b\x45\x14"              // mov    0x14(%ebp),%eax
    "\x89\x04\x24"              // mov    %eax,(%esp)
    "\x8b\x45\x10"              // mov    0x10(%ebp),%eax
    "\xff\xd0"                  // call   *%eax
    "\x83\xf8\x00";             // cmp    $0x0,%eax
    // jl exit
const char READ_STORE[] =
    "\x89\x03";                 // mov    %eax,(%ebx)

const char WRITE[] =
    "\x0f\xb6\x13"              // movzbl (%ebx),%edx
    "\x89\x54\x24\x04"            // mov    %edx,0x4(%esp)
    "\x8b\x45\x0c"                // mov    0xc(%ebp),%eax
    "\x89\x04\x24"                // mov    %eax,(%esp)
    "\x8b\x45\x08"                // mov    0x8(%ebp),%eax
    "\xff\xd0"                  // call   *%eax
    "\x83\xf8\x01";             // cmp    $0x1,%eax

char LOOP_CMP[] =
  "\x80\x3b\x00";                // cmpb   $0x0,(%ebx)

typedef void(*BrainfuckFunction)(bool (write)(void *, char c),
                                void* write_arg,
                                int (read)(void *),
                                void* read_arg,
                                void* memory);

static bool bf_write(void* dummary_arg, char c) {
  putchar(c);
  return true;
};

static int bf_read(void* dummy_arg) {
  int c = getchar();
  if (c == EOF) {
    return 0;
  } else {
    return c;
  }
}

static void add_jne_to_exit(string* code) {
  int relative_address = EXIT_OFFSET - (code->size() + 6);
  *code += "\x0f\x85" + string((char *) &relative_address, sizeof(int));
}

static void add_jl_to_exit(string* code) {
  int relative_address = EXIT_OFFSET - (code->size() + 6);
  *code += "\x0f\x8c" + string((char *) &relative_address, sizeof(int));
}

static void add_jmp_to_offset(int offset, string* code) {
  int relative_address = offset - (code->size() + 5);
  *code += "\xe9" + string((char *) &relative_address, sizeof(int));
}

static void add_jmp_to_exit(string* code) {
  add_jmp_to_offset(EXIT_OFFSET, code);
}

static bool find_loop_end(string::const_iterator loop_start,
                          string::const_iterator string_end,
                          string::const_iterator* loop_end) {
  int level = 1;
  for (string::const_iterator it=loop_start+1; it != string_end; ++it) {
    char c = *it;
    if (*it == '[') {
      level += 1;
    } else if (*it == ']') {
      level -= 1;
      if (level == 0) {
        *loop_end = it;
        return true;
      }
    }
  }
  return false;
}

bool generate_sequence_code(string::const_iterator start,
                            string::const_iterator end,
                            string* code);

bool generate_loop_code(string::const_iterator start,
                        string::const_iterator end,
                        string* code) {
  int loop_start = code->size();
  *code += string(LOOP_CMP, sizeof(LOOP_CMP) - 1);

  int jump_start = code->size();
  // Reserve 6 bytes for je.
  *code += string("\xde\xad\xbe\xef\xde\xad");

  if (!generate_sequence_code(start+1, end, code)) {
    return false;
  }

  add_jmp_to_offset(loop_start, code);  // Jump back to the start of the loop.
  int relative_end_of_loop = code->size() - (jump_start + 6);

  string jump_to_end = "\x0f\x84" + string((char *) &relative_end_of_loop,
                                           sizeof(int));
  code->replace(jump_start, jump_to_end.size(), jump_to_end);

  return true;
}

void generate_left_code(string* code) {
  *code += string(LEFT, sizeof(LEFT) - 1);
}

void generate_right_code(string* code) {
  *code += string(RIGHT, sizeof(RIGHT) - 1);
}

void generate_subtract_code(string* code) {
  *code += string(SUBTRACT, sizeof(SUBTRACT) - 1);
}

void generate_add_code(string* code) {
  *code += string(ADD, sizeof(ADD) - 1);
}

void generate_read_code(string* code) {
  *code += string(READ, sizeof(READ) - 1);
  add_jl_to_exit(code);
  *code += string(READ_STORE, sizeof(READ_STORE) - 1);
}

void generate_write_code(string* code) {
  *code += string(WRITE, sizeof(WRITE) - 1);
  add_jne_to_exit(code);
}

bool generate_sequence_code(string::const_iterator start,
                            string::const_iterator end,
                            string* code) {
  for (string::const_iterator it=start; it != end; ++it) {
    switch (*it) {
      case '<':
        generate_left_code(code);
        break;
      case '>':
        generate_right_code(code);
        break;
      case '-':
        generate_subtract_code(code);
        break;
      case '+':
        generate_add_code(code);
        break;
      case ',':
        generate_read_code(code);
        break;
      case '.':
        generate_write_code(code);
        break;
      case '[':
        string::const_iterator loop_end;
        if (!find_loop_end(it, end, &loop_end)) {
          printf("Test\n");
          fprintf(stderr, "Unable to find loop end in block: %s\n", string(it, end));
          return false;
        }
        if (!generate_loop_code(it, loop_end, code)) {
          return false;
        }
        it = loop_end;
        break;
    }
  }
  return true;
}


BrainfuckProgram::BrainfuckProgram() : executable_(NULL) {}

bool BrainfuckProgram::init(const string& source) {
  string code(PREFIX);

  if (!generate_sequence_code(source.begin(), source.end(), &code)) {
    return false;
  }
  add_jmp_to_exit(&code);

  int required_memory = (code.size() /
                         sysconf(_SC_PAGESIZE) + 1) * sysconf(_SC_PAGESIZE);

  executable_ = mmap(
      NULL,
      required_memory,
      PROT_READ | PROT_WRITE,
      MAP_PRIVATE|MAP_ANON, -1, 0);
  if (executable_ == NULL) {
    fprintf(stderr, "Error making memory executable: %s\n", strerror(errno));
    return false;
  }

  memmove(executable_, code.data(), code.size());

  if (mprotect(executable_, required_memory, PROT_EXEC | PROT_READ) != 0) {
    fprintf(stderr, "mprotect failed: %s\n", strerror(errno));
    return false;
  }
  return true;  
}

void BrainfuckProgram::run(void* memory) {
  ((BrainfuckFunction)executable_)(&bf_write, NULL, &bf_read, NULL, memory);
}
