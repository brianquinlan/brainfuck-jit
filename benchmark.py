#!/usr/bin/python

import os
import os.path
import random
import subprocess
import tempfile
import timeit
import unittest

MODES = ['cag', 'i', 'jit']
EXECUTABLE_PATH = os.path.join(os.curdir, 'bf')

def _generate_brainfuck_no_loop():
    possible_tokens = ['+', '-', '<', '>']
    possible_zero_index_tokens = ['+', '-', '>']
    tokens = []
    offset = 0
    for _ in range(1024*1024):
        if offset:
            token = random.choice(possible_tokens)
        else:
            token = random.choice(possible_zero_index_tokens)

        if token == '<':
            offset -= 1
        elif token == '>':
            offset += 1
        tokens.append(token)
    return ''.join(tokens)

def run(mode, path):
    process = subprocess.Popen(
        [EXECUTABLE_PATH, '--mode=%s' % mode, path],
         stdin=subprocess.PIPE,
         stdout=subprocess.PIPE,
         stderr=subprocess.PIPE)
    process.wait()

def time(mode, path, number):
    return timeit.timeit(
        "run(%r, %r)" % (mode, path),
        setup="from __main__ import run",
        number=number)

def time_all(name, path, number):
    print name
    print '=' * len(name)
    print
    interpreter_time = time('i', path, number)
    print 'Interpreter:     %.2f seconds' % interpreter_time
    compile_and_go_time = time('cag', path, number)
    print 'Compile and Run: %.2f seconds (%.2f%%)' % (
        compile_and_go_time, compile_and_go_time * 100 / interpreter_time)
    jit_time = time('jit', path, number)
    print 'JIT:             %.2f seconds (%.2f%%)' % (
        jit_time, jit_time * 100 / interpreter_time)



if __name__ == '__main__':
    print
    brainfuck_code = _generate_brainfuck_no_loop()
    with tempfile.NamedTemporaryFile(suffix='.b', delete=False) as f:
        f.write(brainfuck_code)
        f.close()
        time_all('Random File (no loops)', f.name, number=100)
        os.unlink(f.name)

    print
    example_path = os.path.join(os.curdir, 'examples', 'decrement_1073741824.b')
    time_all('Decrement (nested loops)', example_path, number=5)
    print



