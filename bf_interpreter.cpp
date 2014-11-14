// Copyright 2014 Brian Quinlan
// See "LICENSE" file for details.

#include <cstdint>
#include <stack>

#include "bf_interpreter.h"

using std::stack;

BrainfuckInterpreter::BrainfuckInterpreter() {}

bool BrainfuckInterpreter::init(string::const_iterator start,
                                string::const_iterator end) {
  start_ = start;
  end_ = end;

  // Build the mapping from the position of the start of a block (i.e. "]") to
  // the character *after* the end of the block.
  stack<string::const_iterator> block_starts;
  for (string::const_iterator it = start; it != end; ++it) {
    if (*it == '[') {
      block_starts.push(it);
    } else if (*it == ']') {
      if (block_starts.size() != 0) {
        const string::const_iterator &loop_start = block_starts.top();
        loop_start_to_after_end_[loop_start] = it+1;
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

void* BrainfuckInterpreter::run(BrainfuckReader reader,
                                void* reader_arg,
                                BrainfuckWriter writer,
                                void* writer_arg,
                                void* memory) {
  uint8_t* byte_memory = reinterpret_cast<uint8_t *>(memory);
  // When processing a "[", push the position of that opcode onto a stack so
  // that we can quickly return to the start of the block when then "]" is
  // interpreted.
  stack<string::const_iterator> return_stack;

  for (string::const_iterator it = start_; it != end_;) {
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
        if (*byte_memory) {
          return_stack.push(it);
           ++it;
        } else {
          it = loop_start_to_after_end_[it];
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
