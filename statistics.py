#!/usr/bin/python3

import matplotlib.pyplot as plt

paths = [
    './mohican.log',
]

names = []
values = []

for path in paths:
    number = 0
    with open(path, 'r') as file:
        while True:
            line = file.readline()

            if not line:
                break

            if line.find('New connection') != -1:
                number += 1
    
    names.append(str(path.index(path) + 1))
    values.append(number)

fig,ax = plt.subplots()

ax.bar(names, values)

ax.set_facecolor('seashell')
ax.set_xlabel('upstreams')
ax.set_ylabel('number of requests')

fig.set_facecolor('floralwhite')
fig.set_figwidth(8)
fig.set_figheight(6)

plt.show()
