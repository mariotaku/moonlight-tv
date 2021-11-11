#!/usr/bin/env node
'use strict';
const {po} = require('gettext-parser');
const {ArgumentParser} = require('argparse');
const path = require('path');
const fs = require('fs');

const parser = new ArgumentParser()
parser.add_argument('-o', '--output', {required: true});
parser.add_argument('locales', {metavar: 'LOCALE', type: String, nargs: '+'})

const args = parser.parse_args();

const outdir = args['output'];
const locales = args['locales'];

for (const locale of locales) {
    let cstrings = {}
    const popath = path.join('i18n', locale, 'messages.po');
    console.log(`Reading ${popath}`);
    const pofile = po.parse(fs.readFileSync(popath));
    const translations = pofile.translations[''];
    for (const msgid in translations) {
        if (!msgid) continue;
        const translation = translations[msgid];
        if (!translation.msgstr) continue;
        cstrings[translation.msgid] = translation.msgstr[0];
    }

    const language = pofile.headers['Language'].split(/[-_]/);
    const lang_path = path.join(outdir, ...language);
    if (!fs.existsSync(lang_path)) {
        fs.mkdirSync(lang_path, {recursive: true});
    }
    fs.writeFileSync(path.join(lang_path, 'cstrings.json'), JSON.stringify(cstrings));
}