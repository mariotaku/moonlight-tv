import fs from "fs/promises";
import asyncTransform from "./async-transform";
import {R_OK} from "constants";
import {pickBy} from 'lodash';

async function codepointsList(path: string, type: string, filter: ((name: string) => boolean)): Promise<Record<string, number>> {
    const content = await fs.readFile(path, {encoding: 'utf-8'});
    switch (type) {
        case '.codepoints': {
            const lines: string[] = content.split('\n');
            return Object.fromEntries(lines.map((line): [string, number] => {
                const split = line.trim().split(' ');
                if (split.length !== 2 || !filter(split[0])) return null;
                return [split[0], Number.parseInt(split[1], 16)];
            }).filter(v => v));
        }
        case '.json': {
            return pickBy(JSON.parse(content), (value, key) => filter(key));
        }
        default: {
            throw new Error(`Unsupported file ${path}`);
        }
    }
}

async function selectedList(path: string): Promise<string[]> {
    const lines = (await fs.readFile(path, {encoding: 'utf-8'})).split('\n');
    return lines.map(line => line.trim()).filter(v => v);
}

export default function codepoints() {
    return asyncTransform(async file => {
        const lstFile = file.clone();
        lstFile.extname = '.list';
        const list = await selectedList(lstFile.path);
        file.list = list;

        const cpFile = file.clone();
        cpFile.extname = '.codepoints';
        try {
            await fs.access(cpFile.path, R_OK)
        } catch (e) {
            cpFile.extname = '.json';
        }
        file.codepoints = await codepointsList(cpFile.path, cpFile.extname, cp => list.includes(cp));
        return file;
    });
}