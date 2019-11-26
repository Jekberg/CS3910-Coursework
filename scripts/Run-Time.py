import time
import subprocess
import sys

# sys.argv[0] self
# sys.argv[1:] command and arguments

start = time.time_ns()

process = subprocess.Popen(sys.argv[1:])
process.wait()

total = time.time_ns() - start
print(total, "ns")
