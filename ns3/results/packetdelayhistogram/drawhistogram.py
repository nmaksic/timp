import os.path
import sys
import shutil

print "leafspine"
os.system("./parserhistogram ../result_leafspine/results_leafspine 5")
os.system("octave --silent --eval packetdelay");
os.system("cp delayhistogram.jpg delayhistogram_leafspine.jpg");
print "torus"
os.system("./parserhistogram ../result_torus/results_torus 3")
os.system("octave --silent --eval packetdelay");
os.system("cp delayhistogram.jpg delayhistogram_torus.jpg");
print "hypercube"
os.system("./parserhistogram ../result_hypercube/results_hypercube 3")
os.system("octave --silent --eval packetdelay");
os.system("cp delayhistogram.jpg delayhistogram_hypercube.jpg");
print "flattened butterfly"
os.system("./parserhistogram ../result_butterfly/results_flattened_butterfly 3")
os.system("octave --silent --eval packetdelay");
os.system("cp delayhistogram.jpg delayhistogram_fb.jpg");
os.system("rm delayhistogram.jpg");
