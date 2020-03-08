self.Module = {
    wasmBinaryFile: 'chunk_png.wasm',
    print: log,
    printErr: logError
};

importScripts('chunk_png.js');

self.onmessage = e => {
    switch (e.data.type) {
        case 'image':
            processImage(e.data);
            break;
    }
};

function processImage(args) {
    const { rgbData, width, height, fileSize, options } = args;
    try {
        log(`Working...`);
        const buffer = Module._malloc(rgbData.byteLength);
        Module.HEAPU8.set(rgbData, buffer);
        if (rgbData.byteLength !== width * height * 4) {
            self.postMessage({ type: 'error', error: `Invalid data length: ${rgbData.byteLength}, expected ${width * height * 4}` });
            return;
        }
        const outputSizePt = Module._malloc(4);
        const result = Module._compress(width, height, buffer, outputSizePt);
        if (result) {
            self.postMessage({ type: 'error', error: `Compression error: ${result}` });
        } else {
            const compressedSize = Module.getValue(outputSizePt, 'i32', false);
            const percentage = (compressedSize / fileSize * 100).toFixed(1);
            log(`Compressed: ${fileSize} -> ${compressedSize} bytes (${percentage}%)`);
            const compressed = new Uint8Array(compressedSize);
            compressed.set(Module.HEAPU8.subarray(buffer, buffer + compressedSize));
            self.postMessage({ type: 'result', result: compressed });
        }
        Module._free(buffer);
        Module._free(outputSizePt);
    } catch (e) {
        logError(e.toString());
    }
}

function log(msg) {
    self.postMessage({ type: 'log', msg });
}

function logError(msg) {
    self.postMessage({ type: 'logError', msg });
}
