#ifndef BF_JIT
#define BF_JIT

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
  class Loop {
   public:
    Loop() : condition_evaluation_count(0) { };
    Loop(string::const_iterator end) : loop_end(end),  condition_evaluation_count(0) { };

    string::const_iterator loop_end;
    uint64_t condition_evaluation_count;
    shared_ptr<BrainfuckCompileAndGo> compiled;
  };

  string::const_iterator start_;
  string::const_iterator end_;

  map<string::const_iterator, Loop> loop_start_to_loop_;
};

#endif  // BF_JIT