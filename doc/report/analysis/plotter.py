

#!/bin/python3

import matplotlib.pyplot as plt
import numpy as np
import math
import sys

if len(sys.argv) < 2:
    print("No file provided :(")
    exit()

logFile = open(sys.argv[1], 'r')
logLines = logFile.readlines()

fig, ax = plt.subplots()

xVals = []
yVals = []

integral = 0
prevValue = 0
prevTime = 0
for line in logLines:
    if line[0] == '#': continue
    line = line.split(",")
    value = float(line[0])
    time = float(line[13])

    xVals.append(time)
    yVals.append(value)

    integral += prevValue * (time - prevTime)
    prevTime = time
    prevValue = value

# Haha imagine having block scope lmao
average = integral / time
print("Average value is {}".format(average))

ax.plot(xVals, yVals)

plt.show()