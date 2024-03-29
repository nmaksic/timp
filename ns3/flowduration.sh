


OLDIS=10
TOPOLOGY=$1

rm flowduration*.txt
rm flowcount*.txt
rm results*.xml

for RUNNUMBER in 1 2 3 4 5
do

   echo "RUNNUMBER $RUNNUMBER"

   ./waf --run "scratch/timpsim --flowecmp=true --oldicount=$OLDIS --topology=$TOPOLOGY --RngRun=$RUNNUMBER"
   rm flowduration1.txt
   rm flowcount1.txt
   cp flowduration.txt flowduration1.txt
   cp flowcount.txt flowcount1.txt
   cp results.xml results1.xml
   cp throughputs.txt throughputs1.txt
   

   ./waf --run "scratch/timpsim --flowecmp=false --routing=1 --oldicount=$OLDIS --topology=$TOPOLOGY --RngRun=$RUNNUMBER"
   rm flowduration2.txt
   rm flowcount2.txt
   cp flowduration.txt flowduration2.txt
   cp flowcount.txt flowcount2.txt
   cp results.xml results2.xml
   cp throughputs.txt throughputs2.txt

   ./waf --run "scratch/timpsim --flowecmp=false --routing=2 --oldicount=$OLDIS --topology=$TOPOLOGY --RngRun=$RUNNUMBER"
   rm flowduration3.txt
   rm flowcount3.txt
   cp flowduration.txt flowduration3.txt
   cp flowcount.txt flowcount3.txt
   cp results.xml results3.xml
   cp throughputs.txt throughputs3.txt

   if [ $1 -eq 4 ] || [ $1 -eq 5 ];
   then

   ./waf --run "scratch/timpsim --flowecmp=false --routing=3 --oldicount=$OLDIS --topology=$TOPOLOGY --RngRun=$RUNNUMBER"
   rm flowduration4.txt
   rm flowcount4.txt
   cp flowduration.txt flowduration4.txt
   cp flowcount.txt flowcount4.txt
   cp results.xml results4.xml
   cp throughputs.txt throughputs4.txt

   ./waf --run "scratch/timpsim --flowecmp=false --routing=4 --oldicount=$OLDIS --topology=$TOPOLOGY --RngRun=$RUNNUMBER"
   rm flowduration5.txt
   rm flowcount5.txt
   cp flowduration.txt flowduration5.txt
   cp flowcount.txt flowcount5.txt
   cp results.xml results5.xml
   cp throughputs.txt throughputs5.txt

   rm simgraph.jpg

   python parserflowduration1.py simgraph.jpg flowduration1.txt flowcount1.txt flowduration2.txt flowcount2.txt  flowduration3.txt flowcount3.txt  flowduration4.txt flowcount4.txt  flowduration5.txt flowcount5.txt  "ECMP" "TIMP" "Contra" "DASH packet" "DASH flowlet"

   else

   rm simgraph.jpg
   python parserflowduration.py simgraph.jpg flowduration1.txt flowcount1.txt flowduration2.txt flowcount2.txt  flowduration3.txt flowcount3.txt  "ECMP" "TIMP" "Contra"

   fi

   if [ $1 -eq 1 ]
   then
      echo Simulation in hypercube topology is finished.
      cp simgraph.jpg simgraph_hypercube_run${RUNNUMBER}.jpg
      
      cp results1.xml results_hypercube_ecmp${RUNNUMBER}.xml
      cp results2.xml results_hypercube_timp${RUNNUMBER}.xml
      cp results3.xml results_hypercube_contra${RUNNUMBER}.xml
      
      cp throughputs1.txt throughputs_hypercube_ecmp${RUNNUMBER}.txt
      cp throughputs2.txt throughputs_hypercube_timp${RUNNUMBER}.txt
      cp throughputs3.txt throughputs_hypercube_contra${RUNNUMBER}.txt
   fi
   if [ $1 -eq 2 ]
   then
      echo Simulation in flattened butterfly topology is finished.
      cp simgraph.jpg simgraph_flattened_butterfly_run${RUNNUMBER}.jpg
      
      cp results1.xml results_flattened_butterfly_ecmp${RUNNUMBER}.xml
      cp results2.xml results_flattened_butterfly_timp${RUNNUMBER}.xml
      cp results3.xml results_flattened_butterfly_contra${RUNNUMBER}.xml
      
      cp throughputs1.txt throughputs_flattened_butterfly_ecmp${RUNNUMBER}.txt
      cp throughputs2.txt throughputs_flattened_butterfly_timp${RUNNUMBER}.txt
      cp throughputs3.txt throughputs_flattened_butterfly_contra${RUNNUMBER}.txt
      
   fi
   if [ $1 -eq 3 ]
   then
      echo Simulation in torus topology is finished.
      cp simgraph.jpg simgraph_torus_run${RUNNUMBER}.jpg
   
      cp results1.xml results_torus_ecmp${RUNNUMBER}.xml
      cp results2.xml results_torus_timp${RUNNUMBER}.xml
      cp results3.xml results_torus_contra${RUNNUMBER}.xml
      
      cp throughputs1.txt throughputs_torus_ecmp${RUNNUMBER}.txt
      cp throughputs2.txt throughputs_torus_timp${RUNNUMBER}.txt
      cp throughputs3.txt throughputs_torus_contra${RUNNUMBER}.txt
      
   fi
   if [ $1 -eq 5 ]
   then
      echo Simulation in leaf spine topology is finished.
      cp simgraph.jpg simgraph_leafspine_run${RUNNUMBER}.jpg
      
      cp results1.xml results_leafspine_ecmp${RUNNUMBER}.xml
      cp results2.xml results_leafspine_timp${RUNNUMBER}.xml
      cp results3.xml results_leafspine_contra${RUNNUMBER}.xml
      cp results4.xml results_leafspine_dashpkt${RUNNUMBER}.xml
      cp results5.xml results_leafspine_dashflowlet${RUNNUMBER}.xml
      
      
      cp throughputs1.txt throughputs_leafspine_ecmp${RUNNUMBER}.txt
      cp throughputs2.txt throughputs_leafspine_timp${RUNNUMBER}.txt
      cp throughputs3.txt throughputs_leafspine_contra${RUNNUMBER}.txt
      cp throughputs3.txt throughputs_leafspine_dashpkt${RUNNUMBER}.txt
      cp throughputs3.txt throughputs_leafspine_dashflowlet${RUNNUMBER}.txt
      
   fi

   if [ $1 -eq 1 ] || [ $1 -eq 2 ] || [ $1 -eq 3 ];
   then
      for PROTOCOL in 1 2 3
      do
         cp flowduration${PROTOCOL}.txt flowduration${PROTOCOL}_${RUNNUMBER}.txt
         cp flowcount${PROTOCOL}.txt flowcount${PROTOCOL}_${RUNNUMBER}.txt
      done
   fi


   if [ $1 -eq 5 ];
   then
      for PROTOCOL in 1 2 3 4 5
      do
         cp flowduration${PROTOCOL}.txt flowduration${PROTOCOL}_${RUNNUMBER}.txt
         cp flowcount${PROTOCOL}.txt flowcount${PROTOCOL}_${RUNNUMBER}.txt
      done
   fi


done


if [ $1 -eq 1 ] || [ $1 -eq 2 ] || [ $1 -eq 3 ];
then
   # calculate average 
   for PROTOCOL in 1 2 3
   do
      python average.py flowduration${PROTOCOL}_1.txt flowcount${PROTOCOL}_1.txt flowduration${PROTOCOL}_2.txt flowcount${PROTOCOL}_2.txt  flowduration${PROTOCOL}_3.txt flowcount${PROTOCOL}_3.txt  flowduration${PROTOCOL}_4.txt flowcount${PROTOCOL}_4.txt  flowduration${PROTOCOL}_5.txt flowcount${PROTOCOL}_5.txt flowduration${PROTOCOL}_avg.txt flowcount${PROTOCOL}_avg.txt
   done

   # drow average
   python parserflowduration.py simgraph.jpg flowduration1_avg.txt flowcount1_avg.txt flowduration2_avg.txt flowcount2_avg.txt  flowduration3_avg.txt flowcount3_avg.txt  "ECMP" "TIMP" "Contra"

   if [ $1 -eq 1 ]
   then
      echo Simulation in hypercube topology is finished.
      cp simgraph.jpg simgraph_hypercube_avg.jpg
   fi
   if [ $1 -eq 2 ]
   then
      echo Simulation in flattened butterfly topology is finished.
      cp simgraph.jpg simgraph_flattened_butterfly_avg.jpg
   fi
   if [ $1 -eq 3 ]
   then
      echo Simulation in torus topology is finished.
      cp simgraph.jpg simgraph_torus_avg.jpg
   fi

fi

if [ $1 -eq 5 ];
then
   # calculate average 
   for PROTOCOL in 1 2 3 4 5
   do
      python average.py flowduration${PROTOCOL}_1.txt flowcount${PROTOCOL}_1.txt flowduration${PROTOCOL}_2.txt flowcount${PROTOCOL}_2.txt  flowduration${PROTOCOL}_3.txt flowcount${PROTOCOL}_3.txt  flowduration${PROTOCOL}_4.txt flowcount${PROTOCOL}_4.txt  flowduration${PROTOCOL}_5.txt flowcount${PROTOCOL}_5.txt flowduration${PROTOCOL}_avg.txt flowcount${PROTOCOL}_avg.txt
   done

   # drow average
   python parserflowduration1.py simgraph.jpg flowduration1_avg.txt flowcount1_avg.txt flowduration2_avg.txt flowcount2_avg.txt  flowduration3_avg.txt flowcount3_avg.txt  flowduration4_avg.txt flowcount4_avg.txt  flowduration5_avg.txt flowcount5_avg.txt  "ECMP" "TIMP" "Contra" "DASH packet" "DASH flowlet"

   if [ $1 -eq 5 ]
   then
      echo Simulation in leaf spine topology is finished.
      cp simgraph.jpg simgraph_leafspine_avg.jpg
   fi

fi



