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

o = subprocess.check_output(['./_test_tree'])

if o == correct:
    print(" >> TEST PASSED!")
    sys.exit(0)

print(" >> TEST FAILED - wrong tree")
sys.exit(1)
