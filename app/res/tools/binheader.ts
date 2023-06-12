import {BufferFile} from "vinyl";
import {camelCase, constantCase, pascalCase, snakeCase} from "change-case";
import asyncTransform from "./async-transform";

export type Case = 'camelCase' | 'PascalCase' | 'snake_case' | 'UPPER_CASE';

export interface Option {
    naming: Case;
    prefix: string;
}

function joinWithCase(segments: string[], naming: Case): string {
    switch (naming) {
        case "camelCase": {
            return camelCase(segments.map(s => pascalCase(s)).join());
        }
        case "PascalCase": {
            return segments.map(s => pascalCase(s)).join();
        }
        case "UPPER_CASE": {
            return segments.map(s => constantCase(s)).join('_');
        }
        case "snake_case": {
            return segments.map(s => snakeCase(s)).join('_');
        }
    }
}

export default function binHeader(option?: Partial<Option>) {
    option = option || {};
    return asyncTransform(async file => {
        if (!file.isBuffer()) {
            throw new Error('Only buffer file is supported!');
        }
        const bf = file as BufferFile;
        const buffer = bf.contents;
        const hexArray = buffer.toJSON().data.map(item => {
            return `0x${item.toString(16).padStart(2, '0')}`
        });
        const naming = option.naming ?? 'snake_case';
        const dataSymbol = joinWithCase([option.prefix, bf.stem, 'data'].filter(v => v), naming);
        const sizeSymbol = joinWithCase([option.prefix, bf.stem, 'size'].filter(v => v), naming);

        let source = '#pragma once\n';
        source += `const unsigned char ${dataSymbol}[] = {\n`;
        const cols = 16;
        for (let i = 0; i < hexArray.length; i += cols) {
            source += `  ${hexArray.slice(i, Math.min(i + cols, hexArray.length)).join(', ')},\n`;
        }
        source += '};\n';
        source += `extern const unsigned char ${dataSymbol}[];\n`;
        source += `#define ${sizeSymbol} ${buffer.length}\n`;

        bf.contents = Buffer.from(source);
        bf.extname = '.h';
    });
}