import subprocess
import sys

# sys.argv[0] self
# sys.argv[1] count
# sys.argv[2:] command and arguments
for i in range(0, int(sys.argv[1])):
    process = subprocess.Popen(sys.argv[2:])
    process.wait()