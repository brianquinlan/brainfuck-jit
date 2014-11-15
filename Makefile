CC=g++
CPPFLAGS=-std=c++11 -Wall -Wextra -O3

all: bf

bf: bf_main.cpp *.cpp *.h
	$(CC) $(CPPFLAGS) -m64 bf_main.cpp bf_compile_and_go.cpp bf_interpreter.cpp bf_jit.cpp -o bf

test: bf
	python test_runner.py

presubmit: test *.cpp *.h
	python tools/cpplint.py *.cpp *.h
	(which pylint >/dev/null && pylint test_runner benchmark) || (which pylint >/dev/null || echo "WARNING: pylint is not installed, skipping Python checks.")

benchmark: bf
	python benchmark.py

clean:
	rm -rf *.o *.pyc bf
