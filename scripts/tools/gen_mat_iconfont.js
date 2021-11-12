#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const convert = require('lv_font_conv/lib/convert');

const {ArgumentParser} = require("argparse");
const parser = new ArgumentParser()
parser.add_argument('-f', '--font', {required: true});
parser.add_argument('-c', '--codepoints', {required: true});
parser.add_argument('-o', '--output', {required: true});
parser.add_argument('-l', '--list', {required: true});
parser.add_argument('-n', '--name', {required: true});
parser.add_argument('-p', '--prefix', {required: true});
parser.add_argument('-s', '--sizes', {nargs: '+', required: true, type: Number});

const args = parser.parse_args();

const codepoints = Object.fromEntries(fs.readFileSync(args.codepoints, {encoding: 'utf-8'}).split('\n').map(line => {
    const split = line.split(' ');
    if (split.length !== 2) return null;
    return [split[0], Number.parseInt(split[1], 16)];
}).filter(v => v));

const list = fs.readFileSync(args.list, {encoding: 'utf-8'}).split('\n').map(line => line.trim())
    .filter(v => v);

const ranges = list.map(line => {
    let codepoint = codepoints[line.trim()];
    if (!codepoint) return null;
    return {range: [codepoint, codepoint, codepoint]};
}).filter(v => v);

async function do_convert() {
    for (size of args.sizes) {
        const files = await convert({
            font: [{
                source_path: args.font,
                source_bin: fs.readFileSync(args.font),
                ranges: ranges
            }],
            format: 'lvgl',
            output: path.join(args.output, `${args.name}_${size}.c`),
            size: size,
            bpp: 4,
            lcd: false,
            lcd_v: false,
            use_color_info: false,
            no_compress: true,
        });
        for (let [filename, data] of Object.entries(files)) {
            let dir = path.dirname(filename);

            if (!fs.existsSync(dir)) {
                fs.mkdirSync(dir, {recursive: true});
            }

            fs.writeFileSync(filename, data);
        }
    }
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