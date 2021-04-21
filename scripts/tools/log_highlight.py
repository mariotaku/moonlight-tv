#!/usr/bin/env python3
import sys
import time
import re

line_pattern = re.compile(r'([\d\.]+) \[(.+)?\]\[(.+)?\].*')


colors = {
    'ERROR': '\033[31m',  # RED
    'WARN': '\033[33m',  # YELLOW
    'INFO': '\033[36m',  # CYAN
    'DEBUG': '\033[34m',  # BLUE
    'RESET': '\033[0m',
}

try:
    while True:
        line = sys.stdin.readline().rstrip('\n')
        match = line_pattern.match(line)
        if not match:
            print(line, end='')
            print(colors['RESET'])
            continue

        print(colors.get(match[3].upper(), ''), end='')
        print(line, end='')
        print(colors['RESET'], flush=True)
except KeyboardInterrupt:
    sys.stdout.flush()
    pass
