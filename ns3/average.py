#
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Natasa Maksic <maksicn@etf.rs>
#
#

import os.path
import sys
import shutil

def sortlist(linestr):
	a=[x.strip() for x in linestr.split(' ')]
	a=a[1:]
	ai = [int(x) for x in a]
	ais = sorted(ai)
	return ais

def average(duration1, count1, duration2, count2, duration3, count3, duration4, count4, duration5, count5, outdurations, outcount):
	
	
	f = open(duration1, 'r+')
	lines = f.readlines()	
	durations1 = sortlist(lines[0]) #lines[0]
	f.close()
	
	f = open(duration2, 'r+')
	lines = f.readlines()	
	durations2 = sortlist(lines[0]) #lines[0]
	f.close()
	
	f = open(duration3, 'r+')
	lines = f.readlines()	
	durations3 = sortlist(lines[0]) #lines[0]
	f.close()
	
	f = open(duration4, 'r+')
	lines = f.readlines()	
	durations4 = sortlist(lines[0]) #lines[0]
	f.close()
	
	f = open(duration5, 'r+')
	lines = f.readlines()	
	durations5 = sortlist(lines[0]) #lines[0]
	f.close()

	average1=[]
	
	print "len "
	print len(durations1)
   
	i = 0
	while i < len(durations1):
		avg = (durations1[i] + durations2[i] + durations3[i] + durations4[i] + durations5[i]) / 5
		average1.append(avg)
		i += 1
		
	print len(average1)

	f = open(outdurations,'w')
	s = ' '.join(str(x) for x in average1)
	print >>f, ' ' + s
	f.close()
	
	
	f = open(count1, 'r+')
	lines = f.readlines()	
	counts = lines[0]
	f.close()
	
	f = open(outcount,'w')
	print >>f, counts
	f.close()

average(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8], sys.argv[9], sys.argv[10], sys.argv[11], sys.argv[12])
