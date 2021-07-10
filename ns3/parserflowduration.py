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
	s = ' '.join(str(x) for x in ais)
	return s

def throughputgraphs(imagename, fajl1, fajl2, fajl3, fajl4, fajl5, fajl6, legend1, legend2, legend3):
	
	myFile = open('dropped.m','w')
	print >>myFile, "function dropped()"
	
	print >>myFile, "graphics_toolkit(\"gnuplot\")"

	f = open(fajl1, 'r+')
	lines = f.readlines()	
	sortedstr = sortlist(lines[0]) #lines[0]
	print >>myFile, "flowduration=[ " + sortedstr + "]"
	f.close()

	f = open(fajl2, 'r+')
	lines = f.readlines()	
	print >>myFile, "flowcount=[ " + lines[0] + "]"
	f.close()

	f = open(fajl3, 'r+')
	lines = f.readlines()	
	sortedstr = sortlist(lines[0]) #lines[0]
	print >>myFile, "flowduration1=[ " + sortedstr + "]"
	f.close()

	f = open(fajl4, 'r+')
	lines = f.readlines()	
	print >>myFile, "flowcount1=[ " + lines[0] + "]"
	f.close()

	f = open(fajl5, 'r+')
	lines = f.readlines()	
	sortedstr = sortlist(lines[0]) #lines[0]
	print >>myFile, "flowduration2=[ " + sortedstr + "]"
	f.close()

	f = open(fajl6, 'r+')
	lines = f.readlines()	
	print >>myFile, "flowcount2=[ " + lines[0] + "]"
	f.close()


	print >>myFile, "plot(flowcount, flowduration, '-', flowcount1, flowduration1, '--', flowcount2, flowduration2, '-.');"
	print >>myFile, "legend('" + legend1 + "', '" + legend2 + "', '" + legend3 + "', 'Location', 'northwest');"
	print >>myFile, "xlabel('Flow duration place');"
	print >>myFile, "ylabel('Duration [us]');"
	print >>myFile, "print -djpg " + imagename
	print >>myFile, "endfunction"
	myFile.close()

	os.system("octave --silent --eval dropped");

print "This is the name of the script: ", sys.argv[0]
print "Number of arguments: ", len(sys.argv)
print "The arguments are: " , str(sys.argv)

throughputgraphs(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8], sys.argv[9], sys.argv[10])

