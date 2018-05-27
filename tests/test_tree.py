#!/usr/bin/env python2

import subprocess
import sys

correct = """/
> folder0
==> folder2
==> file0
==> file1
> folder1
==> folder3
====> file3
> file2
"""

try:
    o = subprocess.check_output(['./_test_tree'])
except subprocess.CalledProcessError as ee:
    print(" >> TEST FAILED - wrong exit code %d" % ee.returncode)
    print(ee.output)
    sys.exit(1)

if o == correct:
    print(" >> TEST PASSED!")
    sys.exit(0)

print(" >> TEST FAILED - wrong tree")
print(o)
sys.exit(1)
