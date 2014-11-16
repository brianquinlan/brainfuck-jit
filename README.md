Brainfuck JIT
=============

## Introduction

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

## First Steps

The first step is to acquire a basic understanding of the Brainfuck language. The ability to write programs in it is optional :-)
- http://en.wikipedia.org/wiki/Brainfuck [to learn Brainfuck commands]
- http://fatiherikli.github.io/brainfuck-visualizer/ [runs Brainfuck in the browser]

Then look at the interpreter implementation:
- https://github.com/brianquinlan/brainfuck-jit/blob/master/bf_interpreter.h
- https://github.com/brianquinlan/brainfuck-jit/blob/master/bf_interpreter.cpp

Once you understand the interpreter implementation, the JIT implementation should then be understandable:
- https://github.com/brianquinlan/brainfuck-jit/blob/master/bf_jit.h
- https://github.com/brianquinlan/brainfuck-jit/blob/master/bf_jit.cpp

The compiler implementation requires a bit of assembly knowledge to understandable:
- https://github.com/brianquinlan/brainfuck-jit/blob/master/bf_compile_and_go.h
- https://github.com/brianquinlan/brainfuck-jit/blob/master/bf_compile_and_go.cpp
