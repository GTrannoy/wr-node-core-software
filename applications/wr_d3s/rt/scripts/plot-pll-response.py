#!/usr/bin/python
import sys
import time
import matplotlib.pyplot as plt

t=[]
phase=[]
y=[]

for l in open("resp.dat","rb").readlines():
    tok = l.split()
    t.append(int(tok[0]))
    phase.append(int(tok[1]))
    y.append(int(tok[2]))
    

for p in phase: 
    print("%04x" % p);

plt.plot(t, phase,label="Phase error")
plt.plot(t, y, label="Drive")
plt.legend(framealpha=0.5)

plt.show()
