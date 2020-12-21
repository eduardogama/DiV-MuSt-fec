#!/usr/bin/python

import sys
import os
import numpy as np
import matplotlib.pyplot as plt
from multiprocessing import Pool


	
rep 	 = 10
stopTime = 600 # 

nodes = int(sys.argv[1]) # 5
users = int(sys.argv[2]) # 10
layer = int(sys.argv[3]) # 1 2 3

nodes = int(sys.argv[1])
users = int(sys.argv[2])
layers = [1,2,3]



print ("Starting nodes =",nodes," users =",users)

def start_simulation(layer):
    for i in range(0,rep):
		os.system('./waf --run \'dash-2-wifi-zone-l%d --stopTime=%d --users=%d --tracing=false --layer=%d --nodes=%d --seed=%d --AdaptationLogicToUse=\"dash::player::%s\"\'' % (layer,stopTime, users, layer, nodes, seed, protocol))

## run
pool = Pool(4)
pool.map(start_simulation, layers)


print("Done.")
