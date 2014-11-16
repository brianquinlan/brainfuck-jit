Brainfuck JIT
=============

I started this project to help myself gain a practical understanding of interpreting, compilation and Just-In-Time compilation (JIT). I chose Brainfuck (http://en.wikipedia.org/wiki/Brainfuck) as the source language because it is simple.

The total size of the entire project (including the interpreter, compiler and JIT) is <1,000 lines of C++ code.

## Getting Started

To get the source code, build it and run the "Hello, world!" example:

```bash
git clone https://github.com/brianquinlan/brainfuck-jit.git
cd brainfuck-jit
make test
./bf --mode=cag examples/hello.b  # Run ./bf --help for a list of execution options.
```
