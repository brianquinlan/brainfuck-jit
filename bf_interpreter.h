// Copyright 2014 Brian Quinlan
// See "LICENSE" file for details.
//
// Implements a BrainfuckRunner that interprets the Brainfuck source one command
// at a time.

#ifndef BF_INTERPRETER_H_
#define BF_INTERPRETER_H_

#include <map>
#include <string>

#include "bf_runner.h"

using std::map;
using std::string;

class BrainfuckInterpreter : public BrainfuckRunner {
 public:
  BrainfuckInterpreter();
  virtual bool init(string::const_iterator start,
                    string::const_iterator end);
  virtual void* run(BrainfuckReader reader,
                    void* reader_arg,
                    BrainfuckWriter writer,
                    void* writer_arg,
                    void* memory);

 private:
  string::const_iterator start_;
  string::const_iterator end_;

  // Maps the position of a Brainfuck block start to the token after the end of
  // the block e.g.
  // ,[..,]
  //  ^    ^
  //  x => y
  map<string::const_iterator, string::const_iterator> loop_start_to_after_end_;
};

#endif  // BF_INTERPRETER_H_
