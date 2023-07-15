import {BufferFile, StreamFile} from "vinyl";
import through2 from "through2";
import * as stream from "stream";

type File = BufferFile | StreamFile;

export default function asyncTransform(fn: (file: BufferFile | StreamFile) => Promise<File | void>): stream.Transform {
    return through2.obj((file, _, cb) => fn(file)
        .then(ret => cb(null, ret || file))
        .catch(e => cb(e)));
}