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
#endif