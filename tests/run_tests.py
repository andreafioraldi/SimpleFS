import os
import re

test_re = re.compile(r'test_[a-z]+(\.py)?$')
tests = filter(test_re.match, os.listdir("."))

res = {}

for t in tests:
    print " >> running", t
    r = os.system("./" + t)
    res[t] = r

passed = filter(lambda x: res[x] == 0, res)
failed = filter(lambda x: res[x] != 0, res)

print
print " >> RESULTS:"
print " passed (%d):" % len(passed), " ".join(map(lambda x: x.split(".")[0], passed))
print " failed (%d):" % len(failed), " ".join(map(lambda x: x.split(".")[0], failed)) 
