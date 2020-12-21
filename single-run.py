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

protocol = sys.argv[4] # RateBasedAdaptationLogic RateAndBufferBasedAdaptationLogic 


print("Initializing ...")


for seed in range(0,rep):
	os.system('./waf --run \'dash-2-wifi-zone-v2 --stopTime=%d --users=%d --tracing=false --layer=%d --nodes=%d --seed=%d --AdaptationLogicToUse=\"dash::player::%s\"\'' % (stopTime, users, layer, nodes, seed, protocol))

print("Done.")

