self.Module = {
    wasmBinaryFile: 'chunk_png.wasm',
    print: log,
    printErr: logError,
};

importScripts('chunk_png.js');

self.onmessage = (e) => {
    switch (e.data.type) {
        case 'image':
            processImage(e.data);
            break;
    }
};

function processImage(args) {
    const { rgbData, width, height, fileSize, options } = args;
    try {
        let result; // the result from each operation. If it's not 0, an error occurred
        log(`Working...`);
        if (rgbData.byteLength !== width * height * 4) {
            self.postMessage({
                type: 'error',
                error: `Invalid data length: ${rgbData.byteLength}, expected ${width * height * 4}`,
            });
            return;
        }

        // the amount of rows to write per chunk
        const chunkHeight = 40;
        // the output buffer is where the resulting png will be stored
        // @TODO: determine a good estimate for the output buffer size, or automatically allocate memory in the C program
        const outputBufferPt = Module._malloc(15 * 1024 * 1024);
        // start initializing the PNG writer
        result = Module._start(width, height, chunkHeight, outputBufferPt);

        if (result) {
            self.postMessage({ type: 'error', error: `Error on start: ${result}` });
            return;
        }

        // write a chunk

        let numChunks = Math.ceil(height / chunkHeight);
        const chunkSize = width * chunkHeight * 4;
        let lastChunkHeight = height % chunkHeight;
        let lastChunkSize = width * lastChunkHeight * 4;
        // in case the last chunk is empty, just remove it
        if(lastChunkHeight === 0) {
            lastChunkSize = chunkSize;
            lastChunkHeight = chunkHeight;
            //numChunks -= 1;
        }

        log(`[JS] Number of chunks to write: ${numChunks} (last chunk height: ${lastChunkHeight})`);

        for (let i = 0; i < numChunks; i++) {
            // get the size of the chunk in bytes
            const isLastChunk = i === numChunks - 1;
            const currentChunkHeight = isLastChunk ? lastChunkHeight : chunkHeight;
            const currentChunkSize = isLastChunk ? lastChunkSize : chunkSize;
            log(`[JS] Writing chunk: ${i} (size: ${currentChunkSize}, height: ${currentChunkHeight})`);

            const chunk = rgbData.subarray(i * chunkSize, i * chunkSize + currentChunkSize);
            // allocate memory in the wasm heap for the chunk
            // @TODO: we can probably allocate a predefined memory range for all chunks, since they are
            // going to be the same size anyways
            const dataPt = Module._malloc(currentChunkSize);
            Module.HEAPU8.set(chunk, dataPt);

            // execute adding the chunk
            result = Module._chunk(currentChunkHeight, 0, dataPt);

            // free the data for this chunk
            // @TODO: when we implement a shared memory range for all chunks we don't have to free the memory here
            Module._free(dataPt);

            if (result) {
                self.postMessage({ type: 'error', error: `Error processing chunk: ${result}` });
                return;
            }
        }

        const outputSizePt = Module._malloc(4);
        // pointer to output data location
        const outputDataPt = Module._malloc(4);

        result = Module._end(outputSizePt, outputDataPt);

        if (result) {
            self.postMessage({ type: 'error', error: `Error processing chunk: ${result}` });
            return;
        }

        // const result = Module._start(width, height, height, 0, buffer, outputSizePt);
        if (result) {
            self.postMessage({ type: 'error', error: `Compression error: ${result}` });
            return;
        }

        // the size of the output PNG in bytes
        const outputSize = Module.getValue(outputSizePt, 'i32', false);
        // location within the heap where the png file starts
        const outputStartLoc = Module.getValue(outputDataPt, 'i32', false);
        log('outputStart: ' + outputStartLoc);
        const percentage = ((outputSize / fileSize) * 100).toFixed(1);
        log(`PNG File: ${fileSize} -> ${outputSize} bytes (${percentage}%)`);

        // copy the data from the wasm heap to a new buffer
        const outputPngBuffer = new Uint8Array(outputSize);
        outputPngBuffer.set(Module.HEAPU8.subarray(outputStartLoc, outputStartLoc + outputSize));

        // free the memory on the wasm heap
        Module._clean();
        Module._free(outputSizePt);
        Module._free(outputDataPt);

        // and send it to the main thread
        self.postMessage({ type: 'result', result: outputPngBuffer });
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
