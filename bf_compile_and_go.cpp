// Copyright 2014 Brian Quinlan
// See "LICENSE" file for details.
//
// Compiles Brainfuck commands into amd64 machine code and executes it. The
// assembly documentation uses Intel syntax (see
// http://en.wikipedia.org/wiki/X86_assembly_language#Syntax) and was assembled
// using https://defuse.ca/online-x86-assembler.htm.
//
// References:
// http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-manual-325462.pdf
// http://ref.x86asm.net/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include <cstdint>

#include "bf_compile_and_go.h"

typedef void*(*BrainfuckFunction)(BrainfuckWriter writer,
                                  void* write_arg,
                                  BrainfuckReader reader,
                                  void* read_arg,
                                  void* memory);

// This is the main entry point for the implementation of "BrainfuckFunction".
// It expects it's arguments to be passed as specified in:
// http://www.x86-64.org/documentation/abi.pdf
// - 3.2  Function Calling Sequence
// - 3.2.3 Parameter Passing
const char START[] =
  // Some registers must be saved by the called function (the callee) and
  // restored on exit if they are changed. Using these registers is
  // convenient because it allows us to call the provided "write" and "read"
  // functions without worrying about our registers being changed.
  // See:
  // http://www.x86-64.org/documentation/abi.pdf "Figure 3.4: Register Usage"
  "\x41\x54"              // push   r12  # r12 will store the "write" arg
  "\x41\x55"              // push   r13  # r13 will store the "write_arg" arg
  "\x41\x56"              // push   r14  # r14 will store the "read" arg
  "\x55"                  // push   rbp  # rbp will store the "read_arg" arg
  "\x53"                  // push   rbx  # rbx will store the "memory" arg

  // Store the passed arguments into a callee-saved register.
  "\x49\x89\xfc"          // mov    r12,rdi   # write function => r12
  "\x49\x89\xf5"          // mov    r13,rsi   # write arg 1 =>  r13
  "\x49\x89\xd6"          // mov    r14,rdx   # read function => r14
  "\x48\x89\xcd"          // mov    rbp,rcx   # read arg 1 => rbp
  "\x4c\x89\xc3";         // mov    rbx,r8    # BF memory => rbx

const char EXIT[] =
  "\x48\x89\xd8"          // mov    rbx,rax   # Store return value
  "\x5b"                  // pop    rbx
  "\x5d"                  // pop    rbp
  "\x41\x5e"              // pop    r14
  "\x41\x5d"              // pop    r13
  "\x41\x5c"              // pop    r12
  "\xc3";                 // retq

// , [part1] rax = read(rdp); if (rax == 0) goto exit; ...
const char READ[] =
  "\x48\x89\xef"          // mov    rdi,rbp,
  "\x41\xff\xd6"          // callq  *%r14
  "\x48\x83\xf8\x00";     // cmp    rax,0
  // <inserted by code>   // jl     exit

// , [part2] ... *rbx = rax;
const char READ_STORE[] =
  "\x88\x03";         // mov    [rbx],al

// . rax = write(r13, rbx); if (rax != 1) goto exit;
const char WRITE[] =
  "\x4c\x89\xef"          // mov    rdi,r13
  "\x48\x0f\xb6\x33"      // movzbq rsi,[%rbx]
  "\x41\xff\xd4"          // callq  *%r12
  "\x48\x83\xf8\x01";     // cmp    rax,1
  // <inserted by code>   // jne    exit

char LOOP_CMP[] =
  "\x80\x3b\x00";         // cmpb   rbx,0


static bool find_loop_end(string::const_iterator loop_start,
                          string::const_iterator string_end,
                          string::const_iterator* loop_end) {
  int level = 1;
  for (string::const_iterator it = loop_start+1; it != string_end; ++it) {
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

void BrainfuckCompileAndGo::add_jne_to_exit(string* code) {
  *code += "\x0f\x85";                                               // jne ...
  uint32_t relative_address = exit_offset_ - (code->size() + 4);
  *code +=  string(reinterpret_cast<char *>(&relative_address), 4);  // ... exit
}

void BrainfuckCompileAndGo::add_jl_to_exit(string* code) {
  *code += "\x0f\x8c";                                               // jl ...
  uint32_t relative_address = exit_offset_ - (code->size() + 4);
  *code +=  string(reinterpret_cast<char *>(&relative_address), 4);  // ... exit
}

void BrainfuckCompileAndGo::add_jmp_to_offset(int offset, string* code) {
  *code += "\xe9";                                                   // jmp ...
  uint32_t relative_address = offset - (code->size() + 4);
  *code +=  string(reinterpret_cast<char *>(&relative_address), 4);  // ... exit
}

void BrainfuckCompileAndGo::add_jmp_to_exit(string* code) {
  add_jmp_to_offset(exit_offset_, code);
}

bool BrainfuckCompileAndGo::generate_loop_code(string::const_iterator start,
                                               string::const_iterator end,
                                               string* code) {
  // Converts a Brainfuck command sequence like this:
  // [<code>]
  // Into this:
  // loop_start:
  //   cmpb   [rbx],0
  //   je     loop_end
  //   <code>
  //   jmp    loop_start
  // loop_end:
  //

  int loop_start = code->size();
  *code += string(LOOP_CMP, sizeof(LOOP_CMP) - 1);

  int jump_start = code->size();
  *code += string("\xde\xad\xbe\xef\xde\xad");  // Reserve 6 bytes for je.

  if (!generate_sequence_code(start+1, end, code)) {
    return false;
  }

  add_jmp_to_offset(loop_start, code);  // Jump back to the start of the loop.

  string jump_to_end = "\x0f\x84";                              // je ...
  uint32_t relative_end_of_loop = code->size() -
      (jump_start + jump_to_end.size() + 4);
  jump_to_end += string(
      reinterpret_cast<char *>(&relative_end_of_loop), 4);      // ... loop_end

  code->replace(jump_start, jump_to_end.size(), jump_to_end);
  return true;
}

void BrainfuckCompileAndGo::generate_read_code(string* code) {
  *code += string(READ, sizeof(READ) - 1);
  add_jl_to_exit(code);
  *code += string(READ_STORE, sizeof(READ_STORE) - 1);
}

void BrainfuckCompileAndGo::generate_write_code(string* code) {
  *code += string(WRITE, sizeof(WRITE) - 1);
  add_jne_to_exit(code);
}

// Converts a table of updates to make to Brainfuck memory (using offsets
// relative to the current datapointer location) into instructions. Resets
// the datapointer offset and offset map.
//
// For example these Brainfuck commands:
// "<<<++>>>--->++><>>+>>>"
//
// Would trigger this call:
// emit_offset_table(
//    &{{-3, 0x02}, {0, 0xfd}, {1, 0x02}, {2, 0x01}}, &5, code)
//
// Which would add these instructions to code:
// addb [rbx-3],0x02   # Update each memory location with a single instruction.
// addb [rbx],0xfd
// addb [rbx+1],0x02
// addb [rbx+2],0x01
// add  rbx,2          # Move the data pointer to it's final offset.
void BrainfuckCompileAndGo::emit_offset_table(
    map<int8_t, uint8_t>* offset_to_change, int8_t* offset, string *code) {
  for (auto it = offset_to_change->begin();
       it != offset_to_change->end();
       ++it) {
    int8_t change_offset = it->first;
    uint8_t change_value = it->second;

    if (change_value == 0) {
      continue;
    }

    *code += "\x80\x43";                                  // addb [rbx+XX],YY
    *code += string(reinterpret_cast<char *>(&change_offset), 1);  // XX
    *code += string(reinterpret_cast<char *>(&change_value), 1);   // YY
  }

  if (*offset != 0) {
    *code += "\x48\x83\xc3";                                // add rbx ...
    *code += string(reinterpret_cast<char *>(offset), 1);   // ... offset
    *offset = 0;
  }
  offset_to_change->clear();
}

bool BrainfuckCompileAndGo::generate_sequence_code(string::const_iterator start,
                                                   string::const_iterator end,
                                                   string* code) {
  int8_t offset = 0;
  // Maps offset relative to the datapointer into the amount to change it.
  map<int8_t, uint8_t> offset_to_change;

  for (string::const_iterator it = start; it != end; ++it) {
    switch (*it) {
      case '<':
        --offset;
        if (offset == 127 || offset == -128) {
          emit_offset_table(&offset_to_change, &offset, code);
        }
        break;
      case '>':
        ++offset;
        if (offset == 127 || offset == -128) {
          emit_offset_table(&offset_to_change, &offset, code);
        }
        break;
      case '-':
        offset_to_change[offset] -= 1;
        break;
      case '+':
        offset_to_change[offset] += 1;
        break;
      case ',':
        emit_offset_table(&offset_to_change, &offset, code);
        generate_read_code(code);
        break;
      case '.':
        emit_offset_table(&offset_to_change, &offset, code);
        generate_write_code(code);
        break;
      case '[':
        emit_offset_table(&offset_to_change, &offset, code);
        string::const_iterator loop_end;
        if (!find_loop_end(it, end, &loop_end)) {
          fprintf(
              stderr,
              "Unable to find loop end in block starting with: %s\n",
              string(it, end).c_str());
          return false;
        }
        if (!generate_loop_code(it, loop_end, code)) {
          return false;
        }
        it = loop_end;
        break;
    }
  }
  // The offset table must be emitted because this function is called
  // recursively to handle loops.
  emit_offset_table(&offset_to_change, &offset, code);
  return true;
}


BrainfuckCompileAndGo::BrainfuckCompileAndGo() : executable_(NULL) {}

bool BrainfuckCompileAndGo::init(string::const_iterator start,
                                 string::const_iterator end) {
  string code(START, sizeof(START) - 1);
  code += "\xeb";  // relative jump;
  code += sizeof(EXIT) - 1;
  exit_offset_ = code.size();
  code += string(EXIT, sizeof(EXIT) - 1);

  if (!generate_sequence_code(start, end, &code)) {
    return false;
  }
  add_jmp_to_exit(&code);

  executable_size_ = (code.size() /
                      sysconf(_SC_PAGESIZE) + 1) * sysconf(_SC_PAGESIZE);

  executable_ = mmap(
      NULL,
      executable_size_,
      PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANON, -1, 0);
  if (executable_ == MAP_FAILED) {
    fprintf(stderr, "Error making memory executable: %s\n", strerror(errno));
    return false;
  }

  memmove(executable_, code.data(), code.size());
  if (mprotect(executable_, executable_size_, PROT_EXEC | PROT_READ) != 0) {
    fprintf(stderr, "mprotect failed: %s\n", strerror(errno));
    return false;
  }

  return true;
}

void* BrainfuckCompileAndGo::run(BrainfuckReader reader,
                                 void* reader_arg,
                                 BrainfuckWriter writer,
                                 void* writer_arg,
                                 void* memory) {
  return ((BrainfuckFunction)executable_)(
      writer, writer_arg, reader, reader_arg, memory);
}

BrainfuckCompileAndGo::~BrainfuckCompileAndGo() {
  if (executable_) {
    if (munmap(executable_, executable_size_) != 0) {
      fprintf(stderr, "munmap failed: %s\n", strerror(errno));
    }
  }
}


