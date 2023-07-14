import asyncTransform from "./async-transform";
import sf from 'subset-font';
import {BufferFile} from "vinyl";

export default function subsetFont(text: string | ((file: File) => string)) {
    return asyncTransform(async file => {
        if (!file.isBuffer()) {
            throw new Error('Only buffer file is supported!');
        }
        const bf = file as BufferFile;
        const buffer = bf.contents;
        bf.contents = await sf(buffer, typeof text === 'function' ? text(file) : text, {
            targetFormat: 'truetype'
        });
        bf.extname = '.ttf';
    });
}