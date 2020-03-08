# WASM PNG Chunk Encoder

Sometimes you need to encode very large png files in the browser. You can use this library for it.

## Libraries used

- [libpng](http://www.libpng.org/pub/png/libpng.html)
- [zlib](http://www.zlib.net)

## Building

Prerequisties:
- Git
- Emscripten
- WebAssembly Toolchain
- CMake

```bash
git clone --recursive https://github.com/rulkens/wasm-chunk-png.git
cd wasm-chunk-png
./build.sh
```

## Why?

## Thanks

I would like to thank user antelle.net for providing the basic code to make this work.

## License

[MIT](LICENSE)
