#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include <string>


#include "bf_jit.h"

typedef void(*BrainfuckFunction)(bool (write)(void *, char c),
                                 void* write_arg,
                                 int (read)(void *),
                                 void* read_arg,
                                 void* memory);

// http://www.x86-64.org/documentation/abi.pdf
// 3.2  Function Calling Sequence
// 3.2.3 Parameter Passing
// http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-manual-325462.pdf
const char START[] =
    // XXX
    "\x55"                      // push   %rbp  # Belongs to the caller
    // Move the call arguments into callee-saved registers so the read and write
    // function can be called without having to worry about them.
    "\x49\x89\xfc"              // mov    %rdi,%r12  # write function => r12
    "\x49\x89\xf5"              // mov    %rsi,%r13  # write arg 1 =>  r13
    "\x49\x89\xd6"              // mov    %rdx,%r14  # read function => r14
    "\x48\x89\xcd"              // mov    %rcx,%rbp  # read arg 1 => r15
    "\x4c\x89\xc3";             // mov    %r8,%rbx   # BF memory => rbx

const char EXIT[] = 
    "\x5d"                      // pop    %rbp
    "\xc3";                     // retq

const char LEFT[] =
    "\x48\x83\xeb\x01";         // sub    $0x1,%rbx

const char RIGHT[] =
    "\x48\x83\xc3\x01";         // add    $0x1,%rbx

const char SUBTRACT[] =
    "\x8a\x03"                  // mov    (%rbx),%al
    "\x2c\x01"                  // sub    $0x1,%al
    "\x88\x03";                 // mov    %al,(%rbx)

const char ADD[] =
    "\x8a\x03"                  // mov    (%rbx),%al
    "\x04\x01"                  // add    $0x1,%al
    "\x88\x03";                 // mov    %al,(%rbx)

const char READ[] =
    "\x48\x89\xef"              // mov    %rbp,%rdi
    "\x41\xff\xd6"              // callq  *%r14
    "\x48\x83\xf8\x00";         // cmp    $0x0,%rax
    // jl exit
const char READ_STORE[] =
    "\x48\x89\x03";             // mov    %rax,(%rbx)

const char WRITE[] =
    "\x4c\x89\xef"              // mov    %r13,%rdi
    "\x48\x0f\xb6\x33"          // movzbq (%rbx),%rsi
    "\x41\xff\xd4"              // callq  *%r12
    "\x48\x83\xf8\x01";         // cmp    $0x1,%rax

char LOOP_CMP[] =
  "\x80\x3b\x00";                // cmpb   $0x0,(%rbx)


static bool bf_write(void*, char c) {
  putchar(c);
  return true;
};

static int bf_read(void*) {
  int c = getchar();
  if (c == EOF) {
    return 0;
  } else {
    return c;
  }
}

static bool find_loop_end(string::const_iterator loop_start,
                          string::const_iterator string_end,
                          string::const_iterator* loop_end) {
  int level = 1;
  for (string::const_iterator it=loop_start+1; it != string_end; ++it) {
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

void BrainfuckProgram::add_jne_to_exit(string* code) {
  int relative_address = exit_offset_ - (code->size() + 6);
  *code += "\x0f\x85" + string((char *) &relative_address, sizeof(int));
}

void BrainfuckProgram::add_jl_to_exit(string* code) {
  int relative_address = exit_offset_ - (code->size() + 6);
  *code += "\x0f\x8c" + string((char *) &relative_address, sizeof(int));
}

void BrainfuckProgram::add_jmp_to_offset(int offset, string* code) {
  int relative_address = offset - (code->size() + 5);
  *code += "\xe9" + string((char *) &relative_address, sizeof(int));
}

void BrainfuckProgram::add_jmp_to_exit(string* code) {
  add_jmp_to_offset(exit_offset_, code);
}

bool BrainfuckProgram::generate_loop_code(string::const_iterator start,
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

void BrainfuckProgram::generate_left_code(string* code) {
  *code += string(LEFT, sizeof(LEFT) - 1);
}

void BrainfuckProgram::generate_right_code(string* code) {
  *code += string(RIGHT, sizeof(RIGHT) - 1);
}

void BrainfuckProgram::generate_subtract_code(string* code) {
  *code += string(SUBTRACT, sizeof(SUBTRACT) - 1);
}

void BrainfuckProgram::generate_add_code(string* code) {
  *code += string(ADD, sizeof(ADD) - 1);
}

void BrainfuckProgram::generate_read_code(string* code) {
  *code += string(READ, sizeof(READ) - 1);
  add_jl_to_exit(code);
  *code += string(READ_STORE, sizeof(READ_STORE) - 1);
}

void BrainfuckProgram::generate_write_code(string* code) {
  *code += string(WRITE, sizeof(WRITE) - 1);
  add_jne_to_exit(code);
}

bool BrainfuckProgram::generate_sequence_code(string::const_iterator start,
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
          fprintf(stderr, "Unable to find loop end in block starting with: %s\n", string(it, end).c_str());
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
  string code(START);
  code += "\xeb";  // relative jump;
  code += strlen(EXIT);
  exit_offset_ = code.size();
  code += EXIT;

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
