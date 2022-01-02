#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const {ArgumentParser} = require("argparse");
const parser = new ArgumentParser()

parser.add_argument('-a', '--appinfo', {required: true})
parser.add_argument('-p', '--pkgfile', {required: true})
parser.add_argument('-o', '--output', {required: true})
parser.add_argument('-i', '--icon', {required: true})
parser.add_argument('-l', '--link', {required: true})

const args = parser.parse_args()

const outfile = args.output

const appinfo = JSON.parse(fs.readFileSync(args.appinfo, {encoding: 'utf-8'}));

const pkghash = crypto.createHash('sha256').update(fs.readFileSync(args.pkgfile)).digest('hex')
const pkgfile = path.basename(args.pkgfile);

fs.writeFileSync(outfile, JSON.stringify({
    'id': appinfo.id,
    'version': appinfo.version,
    'type': appinfo.type,
    'title': appinfo.title,
    'appDescription': appinfo.appDescription,
    'iconUri': args.icon,
    'sourceUrl': args.link,
    'rootRequired': false,
    'ipkUrl': pkgfile,
    'ipkHash': {
        'sha256': pkghash
    }
}));