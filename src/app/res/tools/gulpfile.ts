import {dest, src} from "gulp";
import binheader from "./binheader";
import codepoints from "./codepoints";
import rename from "gulp-rename";
import subsetFont from "./gulp-subset-font";
import minimist, {ParsedArgs} from "minimist";
import symheader from "./symheader";

declare interface Args extends ParsedArgs {
    input: string | string[];
    output: string;
}

const options = minimist<Args>(process.argv.slice(2));

function codepointsMetadata(file: any): Record<string, number> {
    return file.codepoints;
}

async function iconfont() {
    return src(options.input)
        .pipe(codepoints())
        .pipe(subsetFont(file => String.fromCodePoint(...Object.values(codepointsMetadata(file)))))
        .pipe(binheader({naming: 'snake_case', prefix: 'ttf'}))
        .pipe(rename(file => {
            file.basename = 'material_icons_regular_ttf';
        }))
        .pipe(dest(options.output));
}

async function symlist() {
    return src(options.input)
        .pipe(codepoints())
        .pipe(symheader({prefix: 'MAT'}))
        .pipe(rename(file => {
            file.basename = 'material_icons_regular_symbols';
        }))
        .pipe(dest(options.output));
}

async function binaries() {
    return src(options.input)
        .pipe(binheader({naming: 'snake_case', prefix: 'res'}))
        .pipe(dest(options.output));
}

exports['iconfont'] = iconfont;
exports['symlist'] = symlist;
exports['binaries'] = binaries;