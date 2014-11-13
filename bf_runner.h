#ifndef BF_RUNNER
#define BF_RUNNER

#include <string>

using std::string;

class BrainfuckRunner {
 public:
  virtual bool init(string::const_iterator start,
                    string::const_iterator end) = 0;
  virtual void run(void* memory) = 0;
};

#endif  // BF_RUNNER