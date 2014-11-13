CC=g++
CPPFLAGS=-Wall -Wextra

all: bf_jit64

bf_jit64: bf_main.cpp bf_interpreter.cpp bf_compile_and_go.cpp
	$(CC) $(CPPFLAGS) -m64 bf_main.cpp bf_compile_and_go.cpp bf_interpreter.cpp -o bf_jit64

test: bf_jit64
	python test_runner.py

clean:
	rm -rf *.o *.pyc bf_jit64

