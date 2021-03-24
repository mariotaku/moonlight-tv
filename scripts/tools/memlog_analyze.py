#!/usr/bin/env python3
import sys
from collections import Counter

with open(sys.argv[1]) as f:
    lines = f.readlines()

ptr_map = {}

for line in lines:
    segs = line.rstrip().split(' ')
    (src, action, ptr, size) = [*segs, *[''] * (4 - len(segs))]
    if action == 'free':
        ptr_map.pop(ptr, None)
    else:
        ptr_map[ptr] = {'src': src, 'size': int(size)}

counter = Counter(map(lambda x: x['src'], ptr_map.values()))
for (src, occurences) in counter.most_common():
    print(f'{src} => {occurences} allocation(s)')