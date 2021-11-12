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
    const pofile = po.parse(fs.readFileSync(popath));
    const translations = pofile.translations[''];
    for (const msgid in translations) {
        if (!msgid) continue;
        const translation = translations[msgid];
        const msgstr = translation.msgstr && translation.msgstr[0];
        if (!msgstr) continue;
        cstrings[translation.msgid] = msgstr;
    }

    const language = locale.split(/[-_]/);
    const lang_path = path.join(outdir, ...language);
    if (!fs.existsSync(lang_path)) {
        fs.mkdirSync(lang_path, {recursive: true});
    }
    console.log(`${popath} => ${path.join(...language, 'cstrings.json')}`);
    fs.writeFileSync(path.join(lang_path, 'cstrings.json'), JSON.stringify(cstrings));
}