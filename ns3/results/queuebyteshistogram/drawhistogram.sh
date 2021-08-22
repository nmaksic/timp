import os.path
import sys
import shutil

print "leafspine"
os.system("./parserhistogram ../result_leafspine/results_leafspine 5")
os.system("octave --silent --eval queuebytes")
os.system("cp queuebyteshistogram.jpg queuebyteshistogram_leafspine.jpg")
print "torus"
os.system("./parserhistogram ../result_torus/results_torus 3")
os.system("octave --silent --eval queuebytes")
os.system("cp queuebyteshistogram.jpg queuebyteshistogram_torus.jpg")
print "flattened butterfly"
os.system("./parserhistogram ../result_butterfly/results_flattened_butterfly 3")
os.system("octave --silent --eval queuebytes")
os.system("cp queuebyteshistogram.jpg queuebyteshistogram_flattenedbutterfly.jpg")
print "hypercube"
os.system("./parserhistogram ../result_hypercube/results_hypercube 3")
os.system("octave --silent --eval queuebytes")
os.system("cp queuebyteshistogram.jpg queuebyteshistogram_hypercube.jpg")
os.system("rm queuebyteshistogram.jpg")
