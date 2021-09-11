Wasms
-----

This is a POC project to run many wasm apps on a single esp32 board (in my case, M5 Atom Matrix, with a nice neopixel 5x5 display matrix). All I wanted to do - is to run a different wasm app for each pixel, and it (kinda) works! I had to quite a lot of optimizations, and reduce all stacks and memory limits to minimum, but I finally got all of 25 pixels running.

Using `wasm3` as wasm runtime. Apps are written in c (for now), see them in `./wasm` folder.

Try for yourself (you'll need `wasi-sdk` and `wasm-micro-runtime`)
```
export WAMR_DIR="path to wamr installation" ; that's where I've stolen the build script from
./build-apps.sh && ls wasm/*.wasm | xargs -n 1 xxd -i > src/data.h
pio run -t upload
```
