#include <stack>

#include "bf_interpreter.h"

using std::stack;

BrainfuckInterpreter::BrainfuckInterpreter() {}

bool BrainfuckInterpreter::init(string::const_iterator start,
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
        loop_start_to_end_[loop_start] = it+1;
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

static bool bf_write(char c) {
  return putchar(c) != EOF;
};

static int bf_read() {
  int c = getchar();
  if (c == EOF) {
    return 0;
  } else {
    return c;
  }
}

void BrainfuckInterpreter::run(void* memory) {
  char* byte_memory = (char *) memory;
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
        *byte_memory = (char) bf_read();
         ++it;
        break;
      case '.':
        bf_write(*byte_memory);
         ++it;
        break;
      case '[':
        if (*byte_memory) {
          return_stack.push(it);
           ++it;
        } else {
          it = loop_start_to_end_[it];
        }
        break;
      case ']':
        if (return_stack.size() != 0) {
          it = return_stack.top();
          return_stack.pop();
        }
        break;
      default:
        ++it;
        break;
    }
  }
}