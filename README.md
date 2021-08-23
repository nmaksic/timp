
# TIMP - P4 implementation and ns-3 simulation

Contents:
- ns-3 simulation
- TIMP P4 implementation
- ns-2 simulation

---------------------------
Running ns-3 simulation:
---------------------------

- extract ns-3.33.1 from archive file available at https://www.nsnam.org/releases/ns-3-33/
- copy timp folders timp, contra and dash to ns-3.33/src directory
- copy timpsim.cc to ns-3.33/scratch directory
- copy ipv4-global-routing.cc, ipv4-global-routing.h and tcp-tx-buffer.cc to ns-3.33/src/internet/model
- copy files flow-monitor.cc, flow-monitor.h, ipv4-flow-probe.cc, ipv6-flow-probe.cc, ipv6-flow-probe.h to ns-3.33/src/flow-monitor/model
- copy files modulegen__gcc_ILP32.py and modulegen__gcc_LP64.py to ns-3.33/src/flow-monitor/bindings
- copy simscript.sh, calculate_results.sh, flowduration.sh, flowdurationcircle.sh, parseflowduration.py, parseflowduration1.py, parseflowdurationcircle.sh and average.py to ns-3.33 directory
- copy folder results to directory ns-3.33
- in the directory ns-3.33/results copy source folder of pugixml-1.11 (https://github.com/zeux/pugixml) to the following folders: routingoverhead, fct, queuebyteshistogram, packetdelayhistogram and droppedpackets. Then execute compile scripts in those folders.
- set executable permission for scripts: chmod +x simscript.sh flowduration.sh flowdurationcircle.sh calculate_results.sh
- octave should be installed on the computer in order for graphs to be created during simulation
- build ns-3 by executing the following command in ns-allinone-3.33 directory: ./build.py --enable-examples --enable-tests
- execute ./simscript.sh in ns-3.33 directory to execute simulations 
- simulation results will be located in the directory ns-3.33/results

---------------------------
Mininet emulation:
---------------------------

The instalation procedure consists of the following steps:

- download and install P4 tutorials enviroment from https://github.com/p4lang/tutorials.
- create virtual machine by following steps from the https://github.com/p4lang/tutorials
- in the virtual machine download project https://github.com/p4lang/tutorials
- copy directory routingmeasuretimp from the archive routingmeasuretimp.tar.gz to the directory tutorials/exercises

In order to run the emulated network, in the terminal change directory to routingmeasuretimp and execute commands:
make clean
make

This starts the emulator using topology defined in the directory pod-topo. This topology consists of four switches and four hosts:

       s3 -- s4
        | \/ | 
        | /\ |
       s1    s2
      | |    | |
     h1 h2  h3 h4

In order to send packets from host h3 to host h1, in the mininet terminal execute the following command which starts terminals connected to two hosts: 
xterm h1 h3

In the terminal for host h1 execute command:
./receive.py

In the terminal for host h3 execute command to transmit message to host h1:
./send.py 10.0.1.1 "Message to transmit"

Then, PCAP files can be examined in the directory pcaps and P4 logs can be examined in the directory logs.

---------------------------
Running ns-2 simulation:
---------------------------

- download ns-2 from https://sourceforge.net/projects/nsnam/files/allinone/ns-allinone-2.35/ns-allinone-2.35.tar.gz/download
- download Hula implementation from https://www.cs.princeton.edu/~jrex/publications.html#sdn  (link ns-2 in the reference for Hula paper)
- extract hula-ns-2.35.tar.gz. Then rename folder hula-ns2-backup to ns-2.35, copy patch file to folder containing ns-2.35 and apply patch by executing command: patch -s -p0 < patch_file. Provided patch files are fattree.patch and leafspine.patch.
- extract ns-allinone-2.35.tar.gz
- replace original ns-2 folder with Hula implementation: in folder ns-allinone-2.35, erase folder ns-2.35. Than copy patched folder ns-2.35 to ns-allinone-2.35.
- compile ns-allinone-2.35 by executing ./install
- move to folder ns-allinone-2.35/ns-2.35/hula/test and run the simulation by executing the script: python run_dctcp.py
- in order to create graph, copy resulting folders websearch* to folder graph, than execute commands: python parse.py , python formatdata.py. Then execute command gnuplot in terminal, and in gnuplot execute command: load 'tf.gp'. 

