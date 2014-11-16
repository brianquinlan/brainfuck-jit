#!/usr/bin/env python
#
# Copyright 2014 Brian Quinlan
# See "LICENSE" file for details.

# pylint: disable=print-statement

"""Run benchmarks among the various execution modes."""

from __future__ import absolute_import
from __future__ import division

import argparse
import tempfile
import test_runner
import timeit

HEADER = (
#pylint: disable=bad-continuation
"               Interpreter          Compiler              JIT")

TRIALS_HEADER_FORMAT = (
#pylint: disable=bad-continuation
"============================================================================\n"
"Max Loop Nesting: %d\n"
"============================================================================")

TRIAL_FORMAT = (
#pylint: disable=bad-continuation
"Trial %2d:       %6.2f              %6.2f              %6.2f")

TRIAL_SUMMARY_FORMAT = (
#pylint: disable=bad-continuation
"----------------------------------------------------------------------------\n"
"Total (range):  %6.2f              %6.2f (%0.2f-%0.2f)  %6.2f (%0.2f-%0.2f)\n"
)

def time(mode, path, repeat, number):
    times = timeit.repeat(
        ("returncode, _, stderr = run_brainfuck(['--mode=%s', %r]); " +
         "assert returncode == 0, 'returncode = %%d, stderr = %%r' %% (" +
         "      returncode, stderr)") % (mode, path),
        setup="from test_runner import run_brainfuck",
        repeat=repeat,
        number=number)
    return min(times)


def time_all(trial_number, path, repeat, number):
    """Time all execution modes with the given file."""

    interpreter_min_time = time('i', path, repeat, number)
    compile_and_go_min_time = time('cag', path, repeat, number)
    jit_min_time = time('jit', path, repeat, number)

    print TRIAL_FORMAT % (
        trial_number,
        interpreter_min_time,
        compile_and_go_min_time,
        jit_min_time)
    return interpreter_min_time, compile_and_go_min_time, jit_min_time


def main():  # pylint: disable=missing-docstring
    parser = argparse.ArgumentParser(
        description='Benchmark the bf executable')

    parser.add_argument(
        '--repeat', '-r',
        dest='repeat', action='store', type=int, default=5,
        help='the number of times to collect before the minimum is taken')

    parser.add_argument(
        '--number', '-n',
        dest='number', action='store', type=int, default=20,
        help=('the number of times to run each command before collecting the '
              'time'))

    parser.add_argument(
        '--trials', '-t',
        dest='num_trials', action='store', type=int, default=20,
        help=('the of randomly generated code samples to use for each loop '
              'level'))

    options = parser.parse_args()

    print
    print HEADER
    for max_nested_loops in range(3):
        print TRIALS_HEADER_FORMAT % max_nested_loops
        times = []
        for trial_number in range(options.num_trials):
            brainfuck_code = test_runner.generate_brainfuck_code(
                '<>+-[]\n', 1024 * 1024, max_nested_loops)
            # pylint doesn't seem to understand with statements
            # pylint: disable=bad-continuation
            with tempfile.NamedTemporaryFile(
                    suffix='.b', delete=False) as brainfuck_source_file:
                brainfuck_source_file.write(brainfuck_code)
                brainfuck_source_file.close()
                times.append(
                    time_all(trial_number+1,
                             brainfuck_source_file.name,
                             options.repeat,
                             options.number))
        interpreter_mean = sum(i for (i, _, _) in times)
        compiler_mean = sum(c for (_, c, _) in times)
        jit_mean = sum(j for (_, _, j) in times)

        jit_best_relative = min(j/i for (i, _, j) in times)
        jit_worst_relative = max(j/i for (i, _, j) in times)

        compiler_best_relative = min(c/i for (i, c, _) in times)
        compiler_worst_relative = max(c/i for (i, c, _) in times)

        print TRIAL_SUMMARY_FORMAT % (
            interpreter_mean,
            compiler_mean, compiler_best_relative, compiler_worst_relative,
            jit_mean, jit_best_relative, jit_worst_relative)


if __name__ == '__main__':
    main()

