#!/usr/bin/env python3
import sys
import curses
import argparse

from collections import Counter

stdscr = curses.initscr()
curses.noecho()
stdscr.keypad(False)

ptr_map = {}

def main():
    for line in sys.stdin:
        segs = line.rstrip().split(' ')
        (src, action, ptr, size) = [*segs, *[''] * (4 - len(segs))]
        if action == 'free':
            ptr_map.pop(ptr, None)
        else:
            ptr_map[ptr] = {'src': src, 'size': int(size)}

        stdscr.erase()
        
        totalsize = sum(map(lambda x:x['size'], ptr_map.values()))
        stdscr.addstr(f'Total {totalsize} bytes')
        counter = Counter(map(lambda x: x['src'], ptr_map.values()))
        for idx, (src, occurences) in enumerate(counter.most_common(curses.LINES - 1)):
            allocsize = sum(map(lambda x:x['size'], filter(lambda x: x['src'] == src, ptr_map.values())))
            stdscr.addstr(idx + 1, 0, f'{src} => {occurences} allocation(s), {allocsize} bytes')
        stdscr.addch('\n')
        stdscr.refresh()

try:
    main()
except KeyboardInterrupt:
    pass

curses.endwin()

if len(ptr_map) > 0:
    print("Leaked pointers")
    for k, v in ptr_map.items():
        print(f'{k} => {v["src"]}, size: {v["size"]}')