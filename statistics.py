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
                ha='center', va='bottom')


def main():
    paths = [
        './mohican.log',
    ]

    names = []
    values = []

    for path in paths:
        number = int(subprocess.check_output('cat {} | grep "New connection" | wc -l'.format(path), shell=True))

        names.append('№{}'.format(path.index(path) + 1))
        values.append(number)

    fig,ax = plt.subplots()

    ax.bar(names, values)

    ax.set_facecolor('seashell')
    ax.set_xlabel('upstreams', fontsize=12)
    ax.set_ylabel('number of requests', fontsize=12)

    fig.set_facecolor('floralwhite')
    fig.set_figwidth(8)
    fig.set_figheight(6)

    add_label(ax.patches, ax, values, height_factor=1.01)

    plt.show()


if __name__ == '__main__':
    main()