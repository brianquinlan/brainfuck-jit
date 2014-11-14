// Copyright 2014 Brian Quinlan
// See "LICENSE" file for details.
//
// Implements a BrainfuckRunner that interprets the Brainfuck source one opcode
// at a time but will use BrainfuckCompileAndGo to execute loops that are
// run frequently.

#ifndef BF_JIT_H_
#define BF_JIT_H_

#include <map>
#include <string>
#include <memory>

#include "bf_runner.h"
#include "bf_compile_and_go.h"

using std::map;
using std::string;
using std::shared_ptr;

class BrainfuckJIT : public BrainfuckRunner {
 public:
  BrainfuckJIT();
  virtual bool init(string::const_iterator start,
                    string::const_iterator end);
  virtual void* run(BrainfuckReader reader,
                    void* reader_arg,
                    BrainfuckWriter writer,
                    void* writer_arg,
                    void* memory);

 private:
  struct Loop {
    Loop() : condition_evaluation_count(0) { }
    explicit Loop(string::const_iterator after) :
        after_end(after),  condition_evaluation_count(0) { }

    // The position of the Brainfuck token after the end of the loop.
    string::const_iterator after_end;
    // The number of types that the loop condition (e.g. "[") has been
    // evaluated. Note that the count will not be updated after the loop has
    // been JITed.
    uint64_t condition_evaluation_count;
    // The compiled code that represents the loop. Will be NULL until the loop
    // is JITed.
    shared_ptr<BrainfuckCompileAndGo> compiled;
  };

  string::const_iterator start_;
  string::const_iterator end_;

  // Maps the position of a Brainfuck block start to Loop e.g.
  // ,[..,]
  //  ^    ^
  //  x    y  => loop_start_to_loop_[x] = Loop(y);
  map<string::const_iterator, Loop> loop_start_to_loop_;
};

#endif  // BF_JIT_H_
