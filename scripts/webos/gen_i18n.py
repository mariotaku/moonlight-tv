#!/usr/bin/env python3
import json
import re
from argparse import ArgumentParser
from os import path
from pathlib import Path
from typing import Dict, List

import polib


class Args:
    output: str
    locales: List[str]


parser = ArgumentParser()
parser.add_argument('-o', '--output', required=True)
parser.add_argument('locales', metavar='LOCALE', type=str, nargs='+')

args: Args = parser.parse_args(namespace=Args())

outdir = args.output
locales = args.locales

for locale in locales:
    popath = Path('i18n', locale, 'messages.po')
    pofile: polib.POFile = polib.pofile(str(popath))
    cstrings: Dict[str, str] = {}
    for entry in pofile.translated_entries():
        cstrings[entry.msgid] = entry.msgstr
    lang_segs = re.compile(r'[-_]').split(locale)
    lang_dir = Path(args.output, *lang_segs)
    if not lang_dir.exists():
        lang_dir.mkdir(parents=True, exist_ok=True)
    print(f'{popath} => {path.join(*lang_segs, "cstrings.json")}')
    with lang_dir.joinpath('cstrings.json').open('w') as f:
        json.dump(cstrings, f, ensure_ascii=True)
