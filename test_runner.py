#!/usr/bin/env python
#
# Copyright 2014 Brian Quinlan
# See "LICENSE" file for details.

"""Tests for the Brainfuck executable."""

from __future__ import absolute_import

import os
import os.path
import random
import subprocess
import tempfile
import unittest

EXECUTABLE_PATH = os.path.join(os.curdir, 'bf')


def _run_brainfuck(args, stdin=''):
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


class TestExecutable(unittest.TestCase):
    """Tests the portions of the executable not related to BF interpretation."""

    def test_no_arguments(self):
        returncode, stdout, stderr = _run_brainfuck(args=[])
        self.assertEqual(returncode, 1)
        self.assertIn('Usage', stdout)
        self.assertIn('You need to specify exactly one Brainfuck file', stderr)

    def test_help(self):
        returncode, stdout, stderr = _run_brainfuck(args=['-h'])
        self.assertEqual(returncode, 0)
        self.assertIn('Usage', stdout)
        self.assertEqual(stderr, '')

    def test_without_mode(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = _run_brainfuck(args=[test_hello_world])
        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Hello World!\n')
        self.assertEqual(stderr, '')

    def test_with_mode(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = _run_brainfuck(
            args=['--mode=jit', test_hello_world])
        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Hello World!\n')
        self.assertEqual(stderr, '')

    def test_with_bad_mode(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = _run_brainfuck(
            args=['--mode=badmode', test_hello_world])
        self.assertEqual(returncode, 1)
        self.assertEqual(stdout, '')
        self.assertIn('Unexpected mode: --mode=badmode', stderr)

    def test_with_bad_flag(self):
        test_hello_world = os.path.join(os.curdir, 'examples', 'hello.b')

        returncode, stdout, stderr = _run_brainfuck(
            args=['--flag=unknown', test_hello_world])
        self.assertEqual(returncode, 1)
        self.assertEqual(stdout, '')
        self.assertIn('Unexpected argument: --flag=unknown', stderr)

    def test_with_mode_no_file(self):
        returncode, stdout, stderr = _run_brainfuck(args=['--mode=jit'])
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

        return _run_brainfuck(
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
    def _generate_brainfuck_no_loop():
        """Generate random Brainfuck code without "[" or "]"."""
        possible_tokens = ['+', '-', '<', '>', ',', '.']
        possible_zero_index_tokens = ['+', '-', '>', ',', '.']
        tokens = []
        offset = 0
        tokens = []
        offset = 0
        for _ in range(1024):
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

    @staticmethod
    def _generate_input(length):
        return ''.join([chr(random.randrange(256)) for _ in range(length)])

    def _check_consistency_with_fuzz(self):
        brainfuck_code = self._generate_brainfuck_no_loop()
        # Can't require more input than the length of the code (without loops).
        brainfuck_input = self._generate_input(len(brainfuck_code))
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

    def test_consistency_with_fuzz(self):
        for _ in range(100):
            self._check_consistency_with_fuzz()


if __name__ == '__main__':
    unittest.main()
