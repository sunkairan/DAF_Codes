#!/usr/bin/env python
import subprocess
import sys

def print_pass(cmd):
    print ('%-30s %s %s' % (' '.join(cmd), '\t'*3, 'Passed'))

def print_failed(cmd):
    print '*' * 80
    print '%s Failed' % ' '.join(cmd)
    print '*' * 80





TEST_FILES = [['./test_ff'], 
              ['./test_bats'],
              ['./test_bats', '10240'],
              ['python', 'test_case_1.py', 'lnc.pdf'],
              ['python', 'test_case_2.py', 'lnc.pdf'],
              ['python', 'test_case_3.py', 'lnc.pdf']]


if len(sys.argv) == 1:
    count = 1
else:
    count = int(sys.argv[1])

for i in xrange(count):
    print '*' * 40
    for cmd in TEST_FILES:
        ret = subprocess.call(cmd)
        if ret != 0:
            print_failed(cmd)
            break
        else:
            print_pass(cmd)

