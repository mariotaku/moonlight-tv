#!/usr/bin/env python3
import argparse
import json
import os
from os import path

import langcodes
import polib

parser = argparse.ArgumentParser()
parser.add_argument('locales', metavar='LOCALE', type=str, nargs='+')
parser.add_argument('-o', '--output', required=True)

args = parser.parse_args()

outdir = args.output

locales = args.locales

for locale in locales:
    cstrings = {}
    popath = path.join('i18n', locale, 'messages.po')
    print(f'Reading {popath}')
    pofile = polib.pofile(popath)
    for entry in pofile:
        if not entry.msgstr:
            continue
        cstrings[entry.msgid] = entry.msgstr
    language = list(map(lambda x: x[1], langcodes.parse_tag(pofile.metadata['Language'])))
    lang_path = path.join(outdir, *language)
    os.makedirs(lang_path, exist_ok=True)
    with open(path.join(lang_path, 'cstrings.json'), 'w') as f:
        json.dump(cstrings, f, ensure_ascii=False)
