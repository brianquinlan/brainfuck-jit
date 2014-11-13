#ifndef BF_INTERPRETER
#define BF_INTERPRETER

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
  virtual void run(void* memory);

 private:
  string::const_iterator start_;
  string::const_iterator end_;

  map<string::const_iterator, string::const_iterator> loop_start_to_end_;
};

#endif  // BF_INTERPRETER