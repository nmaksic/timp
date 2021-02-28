

OLDIS=10
#for OLDIS in `seq 1 10`;
#do
./waf --run "scratch/mesh-network --flowecmp=true --oldicount=$OLDIS"
#cp results
rm flowduration1.txt
rm flowcount1.txt
cp flowduration.txt flowduration1.txt
cp flowcount.txt flowcount1.txt

./waf --run "scratch/mesh-network --flowecmp=false --oldicount=$OLDIS"
rm flowduration2.txt
rm flowcount2.txt
cp flowduration.txt flowduration2.txt
cp flowcount.txt flowcount2.txt
rm slika.jpg
python parserflowduration.py slika.jpg flowduration1.txt flowcount1.txt flowduration2.txt flowcount2.txt "ECMP" "TIMP"
cp slika.jpg slika_circle_$OLDIS.jpg
#done 
