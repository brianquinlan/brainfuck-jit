CC=g++
CPPFLAGS=-Wall -Wextra

all: bf_jit64

bf_jit64: bf_jit_main.cpp bf_jit64.cpp
	$(CC) $(CPPFLAGS) -m64 bf_jit_main.cpp bf_jit64.cpp -o bf_jit64

test: bf_jit64
	python test_runner.py

clean:
	rm -rf *.o *.pyc bf_jit64

