#!/usr/bin/python

import os
import os.path
import subprocess
import unittest

class TestJIT(unittest.TestCase):


    def run_jit(self, example, stdin=None):
        executable_path = os.path.join(os.curdir, 'bf_jit64')
        example_path = os.path.join(os.curdir, 'examples', example)

        run = subprocess.Popen([executable_path, example_path],
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
        stdoutdata, stderrdata = run.communicate('This should be echoed!')
        return run.returncode, stdoutdata, stderrdata

    def test_hello_world(self):
        returncode, stdout, stderr = self.run_jit('hello.b')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'Hello World!\n')
        self.assertEqual(stderr, '')

    def test_cat(self):
        returncode, stdout, stderr = self.run_jit(
            'cat.b',
            stdin='This should be echoed!')

        self.assertEqual(returncode, 0)
        self.assertEqual(stdout, 'This should be echoed!')
        self.assertEqual(stderr, '')

    def test_unbalanced_block(self):
        returncode, stdout, stderr = self.run_jit('unbalanced_block.b')

        self.assertEqual(returncode, 1)
        self.assertEqual(stdout, '')
        self.assertIn('Unable to find loop end in block starting with: [[[',
                      stderr)



if __name__ == '__main__':
    unittest.main()
