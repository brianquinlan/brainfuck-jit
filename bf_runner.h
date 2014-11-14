#ifndef BF_RUNNER
#define BF_RUNNER

#include <string>

using std::string;

typedef bool (*BrainfuckWriter)(void* writer_arg, char c);
typedef char (*BrainfuckReader)(void* reader_arg);

class BrainfuckRunner {
 public:
  virtual bool init(string::const_iterator start,
                    string::const_iterator end) = 0;
  virtual void* run(BrainfuckReader reader,
  					void* reader_arg,
  					BrainfuckWriter writer,
  					void* writer_arg,
  					void* memory) = 0;
};

#endif  // BF_RUNNER