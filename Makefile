CC=g++
CPPFLAGS=-Wall -Wextra -O3

all: bf

bf: bf_main.cpp *.cpp *.h
	$(CC) $(CPPFLAGS) -m64 bf_main.cpp bf_compile_and_go.cpp bf_interpreter.cpp bf_jit.cpp -o bf

test: bf
	python test_runner.py

benchmark: bf
	python benchmark.py

clean:
	rm -rf *.o *.pyc bf
