#ifndef BF_COMPILE_AND_GO
#define BF_COMPILE_AND_GO

#include <string>

#include "bf_runner.h"

using std::string;

class BrainfuckCompileAndGo : public BrainfuckRunner {
 public:
  BrainfuckCompileAndGo();
  virtual bool init(string::const_iterator start,
                    string::const_iterator end);
  virtual void run(void* memory);

  virtual ~BrainfuckCompileAndGo();
 private:
  int executable_size_;
  void* executable_;
  int exit_offset_;

  void add_jne_to_exit(string* code);
  void add_jl_to_exit(string* code);
  void add_jmp_to_offset(int offset, string* code);
  void add_jmp_to_exit(string* code);
  bool generate_sequence_code(string::const_iterator start,
                              string::const_iterator end,
                              string* code);
  bool generate_loop_code(string::const_iterator start,
                          string::const_iterator end,
                          string* code);
  void generate_left_code(string* code);
  void generate_right_code(string* code);
  void generate_subtract_code(string* code);
  void generate_add_code(string* code);
  void generate_read_code(string* code);
  void generate_write_code(string* code);
};

#endif  // BF_COMPILE_AND_GO