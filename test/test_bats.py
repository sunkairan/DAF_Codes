import subprocess
import sys

if len(sys.argv) == 1:
    print 'Using Packet mode'
else:
    print 'Using Batch mode'

for x in xrange(10):
    print "-" * 50
    if len(sys.argv) == 1:
        subprocess.call(['./test_bats'])
    else:
        subprocess.call(['./test_bats', 'batch'])
