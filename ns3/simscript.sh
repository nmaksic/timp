
./flowdurationcircle.sh # circle topology
./flowduration.sh 5  # leaf spine topology
rm -rf result_leafspine
mkdir result_leafspine
cp flowcount*.txt result_leafspine
cp flowduration*.txt result_leafspine
./flowduration.sh 3  # torus topology
rm -rf result_torus
mkdir result_torus
cp flowcount*.txt result_torus
cp flowduration*.txt result_torus
./flowduration.sh 1  # hypercube topology
rm -rf result_hypercube
mkdir result_hypercube
cp flowcount*.txt result_hypercube
cp flowduration*.txt result_hypercube
./flowduration.sh 2  # flattened butterfly topology
rm -rf result_butterfly
mkdir result_butterfly
cp flowcount*.txt result_butterfly
cp flowduration*.txt result_butterfly



