#!/usr/bin/env python
#
# Copyright 2014 Brian Quinlan
# See "LICENSE" file for details.

# pylint: disable=print-statement

"""Run benchmarks among the various execution modes."""

from __future__ import absolute_import
from __future__ import division

import os
import os.path
import random
import subprocess
import tempfile
import timeit

MODES = ['cag', 'i', 'jit']
EXECUTABLE_PATH = os.path.join(os.curdir, 'bf')

def _generate_brainfuck_no_loop():
    """Generate random Brainfuck code without loops or I/O."""
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

def _run(mode, path):
    process = subprocess.Popen(
        [EXECUTABLE_PATH, '--mode=%s' % mode, path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    process.wait()

def time(mode, path, number):
    return timeit.timeit(
        "_run(%r, %r)" % (mode, path),
        setup="from __main__ import _run",
        number=number)

def time_all(name, path, number):
    """Time all execution modes with the given file."""
    print name
    print '=' * len(name)
    print
    interpreter_time = time('i', path, number)
    print 'Interpreter:     %.2f seconds' % interpreter_time
    compile_and_go_time = time('cag', path, number)
    print 'Compile and Run: %.2f seconds (%.2f%%)' % (
        compile_and_go_time, compile_and_go_time * 100 / interpreter_time)
    jit_time = time('jit', path, number)
    print 'JIT):             %.2f seconds (%.2f%%)' % (
        jit_time, jit_time * 100 / interpreter_time)

def main():  # pylint: disable=missing-docstring
    print
    brainfuck_code = _generate_brainfuck_no_loop()
    with tempfile.NamedTemporaryFile(suffix='.b',
                                     delete=False) as brainfuck_source_file:
        brainfuck_source_file.write(brainfuck_code)
        brainfuck_source_file.close()
        time_all('Random File (no loops)',
                 brainfuck_source_file.name,
                 number=100)
        os.unlink(brainfuck_source_file.name)

    print
    example_path = os.path.join(os.curdir, 'examples', 'decrement_1073741824.b')
    time_all('Decrement (nested loops)', example_path, number=5)
    print

if __name__ == '__main__':
    main()



