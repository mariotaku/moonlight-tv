import asyncTransform from "./async-transform";

export interface Option {
    prefix: string;
}

function codepointsMetadata(file: any): Record<string, number> {
    return file.codepoints;
}

export default function symHeader(option?: Partial<Option>) {
    return asyncTransform(async file => {
        const codepoints: Record<string, number> = codepointsMetadata(file);
        let content = '#pragma once\n\n';
        const encoder = new TextEncoder();
        for (let key in codepoints) {
            const cp: number = codepoints[key];
            const value = Array.from(encoder.encode(String.fromCodePoint(cp)))
                .map(v => `\\x${v.toString(16)}`).join('');
            const name = key.toUpperCase().replace(/[^0-9a-z_]/ig, '_');
            content += `#define ${option.prefix}_SYMBOL_${name} "${value}"\n`;
        }
        file.contents = Buffer.from(content);
        file.extname = '.h';
    })
}