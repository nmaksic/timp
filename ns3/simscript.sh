
./flowdurationcircle.sh # circle topology
rm -rf result_circle
mkdir result_circle
cp flowcount*.txt result_circle
cp flowduration*.txt result_circle
cp results*.xml result_circle
cp results*.txt result_circle
./flowduration.sh 5  # leaf spine topology
rm -rf result_leafspine
mkdir result_leafspine
cp flowcount*.txt result_leafspine
cp flowduration*.txt result_leafspine
cp results*.xml result_leafspine
cp results*.txt result_leafspine
./flowduration.sh 3  # torus topology
rm -rf result_torus
mkdir result_torus
cp flowcount*.txt result_torus
cp flowduration*.txt result_torus
cp results*.xml result_torus
cp results*.txt result_torus
./flowduration.sh 1  # hypercube topology
rm -rf result_hypercube
mkdir result_hypercube
cp flowcount*.txt result_hypercube
cp flowduration*.txt result_hypercube
cp results*.xml result_hypercube
cp results*.txt result_hypercube
./flowduration.sh 2  # flattened butterfly topology
rm -rf result_butterfly
mkdir result_butterfly
cp flowcount*.txt result_butterfly
cp flowduration*.txt result_butterfly
cp results*.xml result_butterfly
cp results*.txt result_butterfly
rm -rf results/result_*
cp -r result_* results
./calculate_results.sh



