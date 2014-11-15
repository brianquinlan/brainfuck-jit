#!/usr/bin/env python
#
# Copyright 2014 Brian Quinlan
# See "LICENSE" file for details.

"""Tests for the Brainfuck executable."""

from __future__ import absolute_import

import functools
import os
import os.path
import random
import subprocess
import tempfile
import time
import unittest

EXECUTABLE_PATH = os.path.join(os.curdir, 'bf')


def _check_datapointer_in_range(tokens, restore_offset):
    offset = 0
    for token in tokens:
        if token == '<':
            offset -= 1
        elif token == '>':
            offset += 1
        assert offset >= 0
    if restore_offset:
        assert offset == 0


def _repeat_for_seconds(seconds):
    """Repeat the decorated function for the given number of seconds."""
    def repeat(func):
        @functools.wraps(func)
        def repeater(*args, **kwds):
            end_time = time.time() + seconds
            while time.time() < end_time:
                func(*args, **kwds)
                repeater.count += 1
        repeater.count = 0
        return repeater
    return repeat


def run_brainfuck(args, stdin=''):
    """Run the Brainfuck executable.

    Args:
        args: A sequence containing the command line arguments to use when
            invoking the executable e.g. ['--mode=i', 'example.b'].
        stdin: The str that should be used for stdin.
    Returns:
        A 3-tuple of:
            o the process return code as an int
            o the string written to stdout by the process
            o the string written to stderr by the process
    """
    run = subprocess.Popen([EXECUTABLE_PATH] + args,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
    stdoutdata, stderrdata = run.communicate(stdin)
    return run.returncode, stdoutdata, stderrdata


def generate_brainfuck_code_without_loops(
        token_set,
        num_tokens,
        restore_offset=False):
    """Generate Brainfuck code (without loops) using the given set of tokens.

    Args:
        token_set: A sequence containing the tokens to use e.g. '<>+-,.'. May
            not contain '[' or ']'.
        num_tokens: The length of the returned string.
        restore_offset: If True then there should be no net movement of the
            data pointer when the code is finished exited.

    Returns:
        A sequence of Brainfuck instructions from the given token_set e.g.
        '+>-<,,.>><<'.
    """
    assert '[' not in token_set
    assert ']' not in token_set

    possible_tokens = list(token_set)
    possible_tokens_no_left = list(set(token_set) - set(['<']))

    tokens = []
    offset = 0
    while len(tokens) < num_tokens:
        if restore_offset:
            offset_fix = offset
            if offset_fix and offset_fix >= len(tokens) - num_tokens + 1:
                if offset > 0:
                    tokens.extend('<' * offset_fix)
                else:
                    tokens.extend('>' * offset_fix)
                offset = 0
                continue

        if offset:
            token = random.choice(possible_tokens)
        else:
            token = random.choice(possible_tokens_no_left)

        if (restore_offset and
                num_tokens - len(tokens) == 1 and
                token in '<>'):
            continue

        if token == '<':
            offset -= 1
        elif token == '>':
            offset += 1
        tokens.append(token)

    _check_datapointer_in_range(tokens, restore_offset)
    assert len(tokens) == num_tokens
    assert set(tokens) <= set(token_set)
    return ''.join(tokens)


_LOOP_TEMPLATE = '-[>%s<-]'
_EMPTY_LOOP_TEMPLATE_LEN = len(_LOOP_TEMPLATE % '')


def generate_brainfuck_code(token_set, num_tokens, max_loop_depth):
    """Generate Brainfuck code using the given set of tokens.

    Args:
        token_set: A sequence containing the tokens to use e.g. '<>+-,.[]'.
        num_tokens: The length of the returned string.
        max_loop_depth: The maximim level of nesting for loops e.g. 2.

    Returns:
        A sequence of Brainfuck instructions from the given token_set e.g.
        '+--[>,.-[>>>>.<<<<-]<-]><'.
    """
    tokens_without_loops = ''.join(set(token_set) - set(['[', ']']))
    if '[' not in token_set or ']' not in token_set:
        max_loop_depth = 0

    code_blocks = []
    remaining_tokens = num_tokens
    while remaining_tokens:
        if (max_loop_depth and
                remaining_tokens >= _EMPTY_LOOP_TEMPLATE_LEN and
                random.choice(['loop', 'code']) == 'loop'):
            code_block = _LOOP_TEMPLATE % (
                generate_brainfuck_code(
                    token_set,
                    random.randrange(
                        remaining_tokens - _EMPTY_LOOP_TEMPLATE_LEN + 1),
                    max_loop_depth - 1))
        else:
            code_block = generate_brainfuck_code_without_loops(
                tokens_without_loops,
                random.randrange(remaining_tokens+1),
                restore_offset=True)

        remaining_tokens -= len(code_block)
        code_blocks.append(code_block)

    tokens = ''.join(code_blocks)
    assert len(tokens) == num_tokens, 'len(tokens) [%d] == num_tokens [%d]' % (
        len(tokens), num_tokens)
    assert set(tokens) <= set(token_set)
    return tokens


class TestExecutable(unittest.TestCase):
    """Tests the portions of the executable not related to BF interpretation."""

    def test_no_arguments(self):
        returncode, stdout, stderr = run_brainfuck(args=[])
        self.assertEqual(returncode, 1)
        self.assertIn('Usage', stdout)
        self.assertIn('You need to specify exactly one Brainfuck file', stderr)

    def test_help(self):
        returncode, stdout, stderr = run_brainfuck(args=['-h'])
        self.assertEqual(returncode, 0)
        self.assertIn('Usage', stdout)
        self.assertEqual(stderr, '')

    def test_without_mode(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = run_brainfuck(args=[test_hello_world])
        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Hello World!\n')
        self.assertEqual(stderr, '')

    def test_with_mode(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = run_brainfuck(
            args=['--mode=jit', test_hello_world])
        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Hello World!\n')
        self.assertEqual(stderr, '')

    def test_with_bad_mode(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = run_brainfuck(
            args=['--mode=badmode', test_hello_world])
        self.assertEqual(returncode, 1)
        self.assertEqual(stdout, '')
        self.assertIn('Unexpected mode: --mode=badmode', stderr)

    def test_with_bad_flag(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = run_brainfuck(
            args=['--flag=unknown', test_hello_world])
        self.assertEqual(returncode, 1)
        self.assertEqual(stdout, '')
        self.assertIn('Unexpected argument: --flag=unknown', stderr)

    def test_with_mode_no_file(self):
        returncode, stdout, stderr = run_brainfuck(args=['--mode=jit'])
        self.assertEqual(returncode, 1)
        self.assertIn('Usage', stdout)
        self.assertIn('You need to specify exactly one Brainfuck file', stderr)


class BrainfuckRunnerTestMixin(object):
    """A abstract class for testing a brainfuck execution mode.

    Subclasses must define a "MODE" class variable corresponding to their mode
    flag e.g. "jit".
    """

    MODE = None

    @classmethod
    def run_brainfuck(cls, brainfuck_example, stdin=None):
        test_brainfuck_path = os.path.join(
            os.curdir, 'examples', brainfuck_example)

        return run_brainfuck(
            ['--mode=%s' % cls.MODE, test_brainfuck_path], stdin)

    def test_hello_world(self):
        returncode, stdout, stderr = self.run_brainfuck('hello.b')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Hello World!\n')
        self.assertEqual(stderr, '')

    def test_cat(self):
        returncode, stdout, stderr = self.run_brainfuck(
            'cat.b',
            stdin='This should be echoed!')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'This should be echoed!')
        self.assertEqual(stderr, '')

    def test_unbalanced_block(self):
        returncode, stdout, stderr = self.run_brainfuck('unbalanced_block.b')

        self.assertEqual(returncode, 1)
        self.assertEqual(stdout, '')
        self.assertIn('Unable to find loop end in block starting with: [++',
                      stderr)

    def test_extra_block_end(self):
        returncode, stdout, stderr = self.run_brainfuck('extra_block_end.b')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Hello World!\n')
        self.assertEqual(stderr, '')

    def test_empty(self):
        returncode, stdout, stderr = self.run_brainfuck('empty.b')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, '')
        self.assertEqual(stderr, '')

    def test_regression1(self):
        returncode, stdout, stderr = self.run_brainfuck(
            'regression1.b', stdin='mM')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Mm')
        self.assertEqual(stderr, '')


# pylint: disable=too-few-public-methods
class TestCompileAndGo(unittest.TestCase, BrainfuckRunnerTestMixin):
    MODE = 'cag'


# pylint: disable=too-few-public-methods
class TestInterpreter(unittest.TestCase, BrainfuckRunnerTestMixin):
    MODE = 'i'


# pylint: disable=too-few-public-methods
class TestJIT(unittest.TestCase, BrainfuckRunnerTestMixin):
    MODE = 'jit'


class ConsistentOutputTest(unittest.TestCase):
    """Check that the various BrainfuckRunners produce consistent output."""

    @staticmethod
    def _generate_input(length):
        return ''.join([chr(random.randrange(256)) for _ in range(length)])

    def _check_consistency_with_code(self, brainfuck_code, brainfuck_input):
        with tempfile.NamedTemporaryFile(
            suffix='.b', delete=False) as brainfuck_source_file:
            brainfuck_source_file.write(brainfuck_code)
            brainfuck_source_file.close()

            stdouts = []
            for klass in [TestCompileAndGo, TestInterpreter, TestJIT]:
                returncode, stdout, stderr = klass.run_brainfuck(
                    brainfuck_source_file.name, stdin=brainfuck_input)
                self.assertEqual(returncode, 0)
                self.assertEqual(stderr, '')
                stdouts.append((klass, stdout))

            reference_klass, reference_stdout = stdouts[0]
            self.longMessage = True  # pylint: disable=invalid-name
            for klass, stdout in stdouts[1:]:
                self.assertSequenceEqual(
                    reference_stdout, stdout,
                    'output for %s does not match output for %s for file %s' % (
                        reference_klass.__name__,
                        klass.__name__,
                        brainfuck_source_file.name))
            os.unlink(brainfuck_source_file.name)

    @_repeat_for_seconds(2)
    def test_consistency_with_random_no_loop_input(self):
        brainfuck_code = generate_brainfuck_code_without_loops(
            '+-<>,.', 80)
        # Can't require more input than the length of the code (without loops).
        brainfuck_input = self._generate_input(len(brainfuck_code))
        self._check_consistency_with_code(brainfuck_code, brainfuck_input)

    @_repeat_for_seconds(2)
    def test_consistency_with_random_loop_input(self):
        brainfuck_code = generate_brainfuck_code('<>+-[].', 80, 2)
        self._check_consistency_with_code(brainfuck_code, '')

if __name__ == '__main__':
    unittest.main()
