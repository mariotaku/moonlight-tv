#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const util = require('util');
const Fontmin = require('fontmin');
const rename = require('gulp-rename');
var vfs = require('vinyl-fs');

const {ArgumentParser} = require("argparse");
const parser = new ArgumentParser()
parser.add_argument('-f', '--font', {required: true});
parser.add_argument('-c', '--codepoints', {required: true});
parser.add_argument('-o', '--output', {required: true});
parser.add_argument('-l', '--list', {required: true});
parser.add_argument('-n', '--name', {required: true});
parser.add_argument('-p', '--prefix', {required: true});

const args = parser.parse_args();

const codepoints = Object.fromEntries(fs.readFileSync(args.codepoints, {encoding: 'utf-8'}).split('\n').map(line => {
    const split = line.split(' ');
    if (split.length !== 2) return null;
    return [split[0], Number.parseInt(split[1], 16)];
}).filter(v => v));

const list = fs.readFileSync(args.list, {encoding: 'utf-8'}).split('\n').map(line => line.trim())
    .filter(v => v);

const ranges = list.map(line => {
    let codepoint = codepoints[line];
    if (!codepoint) return null;
    return {range: [codepoint, codepoint, codepoint]};
}).filter(v => v);

async function do_convert() {
    const fontmin = new Fontmin()
        .src(args.font)
        .use(Fontmin.glyph({
            text: String.fromCodePoint(...list.map(line => codepoints[line]).filter(v => v))
        }))
        .use(rename(`${args.name}.ttf`))
        .use(vfs.dest(args.output));
    await util.promisify(fontmin.run).bind(fontmin)();
    let entries = list.map(key => {
        const codepoint = codepoints[key];
        if (!codepoint) return null;
        const value = Array.from(new Buffer(String.fromCodePoint(codepoint), 'utf-8').values())
            .map(v => `\\x${v.toString(16)}`).join('');
        return `#define ${args.prefix}_${key.toUpperCase()} "${value}"`;
    }).filter(v => v);
    let symbols_lines = ['#pragma once', '', ...entries];
    fs.writeFileSync(path.join(args.output, `${args.name}_symbols.h`), symbols_lines.join('\n'));
}

do_convert();