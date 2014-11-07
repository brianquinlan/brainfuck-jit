#ifndef BF_JIT
#define BF_JIT

#include <string>

using std::string;

class BrainfuckReader {
 public:
  virtual int read() = 0;
};

class BrainfuckWriter {
 public:
  virtual bool write(char c) = 0;
};

class BrainfuckProgram {
 public:
  BrainfuckProgram();
  bool init(const string& source);
  void run(void* memory);

 private:
  void* executable_;
};
#endif