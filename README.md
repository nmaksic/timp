
# TIMP - P4 implementation and ns-3 simulation

Contents:
- TIMP P4 implementation
- ns-3 simulation
- ns-2 simulation

Running ns-3 simulation:
- extract ns-3.30.1 from archive file available at https://www.nsnam.org/releases/ns-3-30/
- copy timp folder to ns-3.30.1/src directory
- copy mesh-network.cc, torus-network.cc, hipercube-network.cc, fb-network.cc and timp-circle-network.cc to ns-3.30.1/scratch directory
- copy ipv4-global-routing.cc and ipv4-global-routing.h to ns-3.30.1/src/internet/model
- copy parseflowduration.py, flowduration-mesh.sh, flowduration-torus.sh, flowduration-hypercube.sh and flowduration-fb.sh to ns-3.30.1 directory
- configure ns-3 by executing the following command in ns-3.30.1 directory: ./waf configure
- build ns-3 by executing the following command in ns-3.30.1 directory: ./waf build
- execute one of the flowduration scripts in ns-3.30.1 directory in order to run the simulation and obtain graph for ECMP and TIMP flow execution times

Running ns-2 simulation:
- download ns-2 from https://sourceforge.net/projects/nsnam/files/allinone/ns-allinone-2.35/ns-allinone-2.35.tar.gz/download
- download Hula implementation from https://www.cs.princeton.edu/~jrex/publications.html#sdn  (link ns-2 in the reference for Hula paper)
- extract hula-ns-2.35.tar.gz. Than rename folder hula-ns2-backup to ns-2.35, copy patch file to folder containing ns-2.35 and apply patch by executing command: patch -s -p0 < patch_file. Provided patch files are fattree.patch and leafspine.patch.
- extract ns-allinone-2.35.tar.gz
- replace original ns-2 folder with Hula implementation: in folder ns-allinone-2.35, erase folder ns-2.35. Than copy patched folder ns-2.35 to ns-allinone-2.35.
- compile ns-allinone-2.35 by executing ./install
- move to folder ns-allinone-2.35/ns-2.35/hula/test and run the simulation by executing the script: python run_dctcp.py
- in order to create graph, copy resulting folders websearch* to folder graph, than execute commands: python parse.py , python formatdata.py. Then execute command gnuplot in terminal, and in gnuplot execute command: load 'tf.gp'. 

