
TOPOLOGY=4
OLDIS=2

rm flowduration*.txt
rm flowcount*.txt
rm results*.xml

./waf --run "scratch/timpsim --flowecmp=true --oldicount=$OLDIS --topology=$TOPOLOGY --backgroundtraffic=2"
rm flowduration1.txt
rm flowcount1.txt
cp flowduration.txt flowduration1.txt
cp flowcount.txt flowcount1.txt
cp results.xml results_ecmp.xml
cp throughputs.txt throughputs_ecmp.txt

./waf --run "scratch/timpsim --flowecmp=false --routing=1 --oldicount=$OLDIS --topology=$TOPOLOGY --backgroundtraffic=2"
rm flowduration2.txt
rm flowcount2.txt
cp flowduration.txt flowduration2.txt
cp flowcount.txt flowcount2.txt
cp results.xml results_timp.xml
cp throughputs.txt throughputs_timp.txt

rm simgraph.jpg
python parserflowdurationcircle.py simgraph.jpg flowduration1.txt flowcount1.txt flowduration2.txt flowcount2.txt "ECMP" "TIMP"
cp simgraph.jpg simgraph_circle.jpg

