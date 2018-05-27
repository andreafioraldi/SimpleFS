import os
import re
import subprocess

test_re = re.compile(r'test_[a-z]+(\.py)?$')
tests = filter(test_re.match, os.listdir("."))

res = {}

print " ==== INTEGRITY TESTS ===="

for t in tests:
    print " >> running", t
    r = os.system("./" + t)
    res[t] = r

passed = filter(lambda x: res[x] == 0, res)
failed = filter(lambda x: res[x] != 0, res)

pipes = subprocess.Popen("valgrind --help", stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
std_out, std_err = pipes.communicate()

if pipes.returncode == 0: #valgrind in path
    test_re = re.compile(r'(_)?test_[a-z]+$')
    tests = filter(test_re.match, os.listdir("."))
    
    print
    print " ==== MEMORY LEAKS TESTS ===="
    res = []
    
    for t in tests:
        print " >> running", t
        pipes = subprocess.Popen("valgrind --leak-check=full ./" + t, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        std_out, std_err = pipes.communicate()
        if "All heap blocks were freed" in std_err:
            print " >> NO LEAKS"
        else:
            print " >> MEMORY LEAKS... DOH"
            print std_err
            res.append(t)
    
    print
    print " ==== MEMORY LEAKS REPORT ===="
    print " has leak (%d):" % len(res), " ".join(res) 

print
print " ==== INTEGRITY TESTS RESULTS ===="
print " passed (%d):" % len(passed), " ".join(map(lambda x: x.split(".")[0], passed))
print " failed (%d):" % len(failed), " ".join(map(lambda x: x.split(".")[0], failed)) 
print

