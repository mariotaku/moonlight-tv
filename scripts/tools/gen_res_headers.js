const {snakeCase} = require('snake-case');

const {ArgumentParser} = require("argparse");
const fs = require("fs");
const path = require("path");
const parser = new ArgumentParser()
parser.add_argument('-i', '--input', {nargs: '+', required: true});
parser.add_argument('-o', '--output', {required: true});

const {input, output} = parser.parse_args();

function writeHeader(p) {
    const basename = snakeCase(path.basename(p));

    const content = fs.readFileSync(p);
    const stat = fs.statSync(p);

    let source = '#pragma once\n';
    source += `const unsigned char res_${basename}_data[] = {\n`;
    for (let i = 0; i < content.length; i += 20) {
        let seg = content.toString('hex', i, Math.min(i + 20, content.length));
        source += ' ';
        for (let j = 0; j < seg.length; j += 2) {
            source += ` 0x${seg.substring(j, j + 2)},`;
        }
        source += '\n';
    }
    source += '};\n';
    source += `extern const unsigned char res_${basename}_data[];\n`;
    source += `#define res_${basename}_size ${stat.size}\n`;

    fs.writeFileSync(path.join(output, `${basename}.h`), source);
}

for (const item of input) {
    writeHeader(item);
}