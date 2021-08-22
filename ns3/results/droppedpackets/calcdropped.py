import os.path
import sys
import shutil

os.system("echo leafspine > droppedpackets.txt")
os.system("./parserdropped ../result_leafspine/results_leafspine 5 >> droppedpackets.txt")
os.system("echo torus >> droppedpackets.txt")
os.system("./parserdropped ../result_torus/results_torus 3 >> droppedpackets.txt")
os.system("echo flattened butterfly >> droppedpackets.txt")
os.system("./parserdropped ../result_butterfly/results_flattened_butterfly 3 >> droppedpackets.txt")
os.system("echo hypercube >> droppedpackets.txt")
os.system("./parserdropped ../result_hypercube/results_hypercube 3 >> droppedpackets.txt")
os.system("cat droppedpackets.txt")
