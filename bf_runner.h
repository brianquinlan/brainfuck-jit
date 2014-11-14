// Copyright 2014 Brian Quinlan
// See "LICENSE" file for details.
//
// An abstract interface for classes that can execute Brainfuck code (see
// http://en.wikipedia.org/wiki/Brainfuck).

#ifndef BF_RUNNER_H_
#define BF_RUNNER_H_

#include <string>

using std::string;

typedef bool (*BrainfuckWriter)(void* writer_arg, char c);
typedef char (*BrainfuckReader)(void* reader_arg);

class BrainfuckRunner {
 public:
  // Initialize the runner using the Brainfuck opcodes between the given
  // iterators. Returns false if the Brainfuck code is invalid (i.e. there
  // is a "[" with a matching "]") or there is another initialization error.
  virtual bool init(string::const_iterator start,
                    string::const_iterator end) = 0;

  // Runs the Brainfuck code given in "init" using the provided memory.
  // When "," is evaluated, call reader(reader_arg).
  // When "." is evaluated, call writer(writer_arg, <char to write>)
  // The return value is the location of the data pointer
  // (see http://en.wikipedia.org/wiki/Brainfuck#Commands) when the code is
  // finished being executed.
  virtual void* run(BrainfuckReader reader,
                    void* reader_arg,
                    BrainfuckWriter writer,
                    void* writer_arg,
                    void* memory) = 0;
};

#endif  // BF_RUNNER_H_
