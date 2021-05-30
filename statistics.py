#!/usr/bin/python3


import subprocess
import matplotlib.pyplot as plt


def add_label(rects, ax, labels=None, height_factor=1.01):
    for i, rect in enumerate(rects):
        height = rect.get_height()
        if labels is not None:
            try:
                label = labels[i]
            except (TypeError, KeyError):
                label = ' '
        else:
            label = '%d' % int(height)
        ax.text(rect.get_x() + rect.get_width()/2., height_factor*height,
                '{}'.format(label),
                ha='center', va='bottom', color='white')


def main():
    paths = [
        './mohican.log',
    ]

    names = []
    values = []

    count = 1
    for path in paths:
        number = int(subprocess.check_output('cat {} | grep "New connection" | wc -l'.format(path), shell=True))

        names.append('№{}'.format(count))
        values.append(number)

        count += 1

    fig,ax = plt.subplots()

    for pos in ax.spines:
        ax.spines[pos].set_color('white')
    
    ax.tick_params('both', colors='white')

    ax.bar(names, values, color='forestgreen')

    ax.set_facecolor('black')
    ax.set_xlabel('upstreams', fontsize=12, color='white')
    ax.set_ylabel('number of requests', fontsize=12, color='white')

    fig.set_facecolor('black')
    fig.set_figwidth(8)
    fig.set_figheight(6)

    add_label(ax.patches, ax, values, height_factor=1.01)

    plt.show()


if __name__ == '__main__':
    main()