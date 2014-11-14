#!/usr/bin/python

import os
import os.path
import random
import subprocess
import tempfile
import unittest

class BrainfuckRunnerTests(object):

    def run_brainfuck(self, example, stdin=None):
        raise NotImplementedError()


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
        returncode, stdout, stderr = self.run_brainfuck('regression1.b', stdin='mM')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Mm')
        self.assertEqual(stderr, '')

class TestCompileAndGo(unittest.TestCase, BrainfuckRunnerTests):
    @staticmethod
    def run_brainfuck(example, stdin=None):
        executable_path = os.path.join(os.curdir, 'bf')
        example_path = os.path.join(os.curdir, 'examples', example)

        run = subprocess.Popen([executable_path, '--mode=cag', example_path],
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
        stdoutdata, stderrdata = run.communicate(stdin)
        return run.returncode, stdoutdata, stderrdata


class TestInterpreter(unittest.TestCase, BrainfuckRunnerTests):
    @staticmethod
    def run_brainfuck(example, stdin=None):
        executable_path = os.path.join(os.curdir, 'bf')
        example_path = os.path.join(os.curdir, 'examples', example)

        run = subprocess.Popen([executable_path, '--mode=i', example_path],
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
        stdoutdata, stderrdata = run.communicate(stdin)
        return run.returncode, stdoutdata, stderrdata


class TestJIT(unittest.TestCase, BrainfuckRunnerTests):
    @staticmethod
    def run_brainfuck(example, stdin=None):
        executable_path = os.path.join(os.curdir, 'bf')
        example_path = os.path.join(os.curdir, 'examples', example)

        run = subprocess.Popen([executable_path, '--mode=jit', example_path],
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
        stdoutdata, stderrdata = run.communicate(stdin)
        return run.returncode, stdoutdata, stderrdata


class ConsistentOutputTest(unittest.TestCase):
    def _generate_brainfuck_no_loop(self):
      possible_tokens = ['+', '-', '<', '>', ',', '.']
      possible_zero_index_tokens = ['+', '-', '>', ',', '.']
      tokens = []
      offset = 0
      for _ in range(1024):
          if tokens.count('<') >= tokens.count('>'):
              tokens.append(random.choice(possible_zero_index_tokens))
          else:
              tokens.append(random.choice(possible_tokens))
      return ''.join(tokens)

    def _generate_input(self, length):
      return ''.join([chr(random.randrange(256)) for _ in range(length)])

    def _check_consistency_with_fuzz(self):
      brainfuck_code = self._generate_brainfuck_no_loop()
      # Can't require more input than the length of the code (without loops).
      brainfuck_input = self._generate_input(len(brainfuck_code))
      with tempfile.NamedTemporaryFile(suffix='.b', delete=False) as f:
        f.write(brainfuck_code)
        f.close()

        stdouts = []
        for klass in [TestCompileAndGo, TestInterpreter, TestJIT]:
            returncode, stdout, stderr = klass.run_brainfuck(
                f.name, stdin=brainfuck_input)
            self.assertEqual(returncode, 0)
            self.assertEqual(stderr, '')
            stdouts.append((klass, stdout))

        reference_klass, reference_stdout = stdouts[0]
        self.longMessage = True
        for klass, stdout in stdouts[1:]:
            self.assertSequenceEqual(
                reference_stdout, stdout,
                'output for %s does not match output for %s for file %s' % (
                    reference_klass.__name__, klass.__name__, f.name))
        os.unlink(f.name)

    def test_consistency_with_fuzz(self):
      for _ in range(100):
        self._check_consistency_with_fuzz()


if __name__ == '__main__':
    unittest.main()
