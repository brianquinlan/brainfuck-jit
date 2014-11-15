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
import tempfile
import test_runner
import timeit

MODES = ['cag', 'i', 'jit']
EXECUTABLE_PATH = os.path.join(os.curdir, 'bf')


def time(mode, path, number):
    return timeit.timeit(
        ("returncode, _, _ = run_brainfuck(['--mode=%s', %r]); " +
         "assert returncode == 0") % (mode, path),
        setup="from test_runner import run_brainfuck",
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
    print 'JIT:             %.2f seconds (%.2f%%)' % (
        jit_time, jit_time * 100 / interpreter_time)


def main():  # pylint: disable=missing-docstring
    print
    brainfuck_code = test_runner.generate_brainfuck_code_without_loops(
        '<>+-\n', 1024 * 1024)
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

