#include <cstdint>
#include <stack>

#include "bf_jit.h"

using std::stack;

BrainfuckJIT::BrainfuckJIT() {}

bool BrainfuckJIT::init(string::const_iterator start,
                        string::const_iterator end) {
  start_ = start;
  end_ = end;

  stack<string::const_iterator> block_starts;

  for (string::const_iterator it=start; it != end; ++it) {
    if (*it == '[') {
      block_starts.push(it);
    } else if (*it == ']') {
      if (block_starts.size() != 0) {
        const string::const_iterator &loop_start = block_starts.top();
        loop_start_to_loop_[loop_start] = Loop(it+1);
        block_starts.pop();
      }
    }
  }

  if (block_starts.size() != 0) {
    fprintf(
        stderr,
        "Unable to find loop end in block starting with: %s\n",
        string(block_starts.top(), end).c_str());
    return false;
  }
  return true;
}

void* BrainfuckJIT::run(BrainfuckReader reader,
                        void* reader_arg,
                        BrainfuckWriter writer,
                        void* writer_arg,
                        void* memory) {
  uint8_t* byte_memory = (uint8_t *) memory;
  stack<string::const_iterator> return_stack;

  for (string::const_iterator it=start_; it != end_;) {
    switch (*it) {
      case '<':
        --byte_memory;
         ++it;
        break;
      case '>':
        ++byte_memory;
         ++it;
        break;
      case '-':
        *byte_memory -= 1;
         ++it;
        break;
      case '+':
        *byte_memory += 1;
         ++it;
        break;
      case ',':
        *byte_memory = reader(reader_arg);
         ++it;
        break;
      case '.':
        writer(writer_arg, *byte_memory);
         ++it;
        break;
      case '[':
        { 
          Loop &loop = loop_start_to_loop_[it];
          ++loop.condition_evaluation_count;

          if (loop.compiled == nullptr &&
              loop.condition_evaluation_count > 20) {
            shared_ptr<BrainfuckCompileAndGo> compiled(
              new BrainfuckCompileAndGo());
            string::const_iterator compilation_end(loop.loop_end);

            if (!compiled->init(it, compilation_end)) {
              fprintf(stderr,
                      "Unable to compile code: %s\n",
                      string(it, compilation_end).c_str());
            } else {
              loop.compiled = compiled;
            }
          }

          if (loop.compiled) {
            byte_memory = (uint8_t *) loop.compiled->run(reader,
                                                         reader_arg,
                                                         writer,
                                                         writer_arg,
                                                         byte_memory);
            it = loop_start_to_loop_[it].loop_end;
          } else if (*byte_memory) {
            return_stack.push(it);
            ++it;
          } else {
            it = loop_start_to_loop_[it].loop_end;
          }
        }
        break;
      case ']':
        if (return_stack.size() != 0) {
          it = return_stack.top();
          return_stack.pop();
        } else {
          ++it;
        }
        break;
      default:
        ++it;
        break;
    }
  }
  return byte_memory;
}