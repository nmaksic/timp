import os.path
import sys
import shutil

os.system("rm *.dat")
print "leafspine"
os.system("./parserfct ../result_leafspine/results_leafspine 5")
os.system("octave --silent --eval fct")
os.system("cp fct.jpg fct_leafspine.jpg")
os.system("rm -fr leafspine")
os.system("mkdir leafspine")
os.system("cp *.dat leafspine")
os.system("rm *.dat")
print "torus"
os.system("./parserfct ../result_torus/results_torus 3")
os.system("octave --silent --eval fct")
os.system("cp fct.jpg fct_torus.jpg")
os.system("rm -fr torus")
os.system("mkdir torus")
os.system("cp *.dat torus")
os.system("rm *.dat")
print "flattened butterfly"
os.system("./parserfct ../result_butterfly/results_flattened_butterfly 3")
os.system("octave --silent --eval fct")
os.system("cp fct.jpg fct_flattened_butterfly.jpg")
os.system("rm -fr fb")
os.system("mkdir fb")
os.system("cp *.dat fb")
os.system("rm *.dat")
print "hypercube"
os.system("./parserfct ../result_hypercube/results_hypercube 3")
os.system("octave --silent --eval fct")
os.system("cp fct.jpg fct_hypercube.jpg")
os.system("rm -fr hypercube")
os.system("mkdir hypercube")
os.system("cp *.dat hypercube")
os.system("rm -fr fct.jpg")
