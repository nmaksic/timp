import os.path
import sys
import shutil

os.system("echo leafspine > routing_overhead.txt")
os.system("./parserroutingovh ../result_leafspine/results_leafspine 5 >> routing_overhead.txt")
os.system("echo torus >> routing_overhead.txt")
os.system("./parserroutingovh ../result_torus/results_torus 3 >> routing_overhead.txt")
os.system("echo hypercube >> routing_overhead.txt")
os.system("./parserroutingovh ../result_hypercube/results_hypercube 3 >> routing_overhead.txt")
os.system("echo 'flattened butterfly' >> routing_overhead.txt")
os.system("./parserroutingovh ../result_butterfly/results_flattened_butterfly 3 >> routing_overhead.txt")
os.system("cat routing_overhead.txt")
