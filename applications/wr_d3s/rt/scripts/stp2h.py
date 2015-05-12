#!/usr/bin/python


# Trivial utility to convert Analog Devices 'STP' files (setup register values)
# from AD95xx eval software to C Code

import re
import sys
filename="1.stp"
#print("{
first = True
for l in open(filename,"rb").readlines():
    m = re.match("\"(\w{2})\",\"\w+\",\"(\w{2})\"", l)
    if m:
	if not first:
	    sys.stdout.write(",\n");
        first = False
        sys.stdout.write("{0x%s, 0x%s}"%(m.group(1), m.group(2)))