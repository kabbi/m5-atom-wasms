Wasms
-----

This is a POC project to run many wasm apps on a single esp32 board (in my case, M5 Atom Matrix, with a nice neopixel 5x5 display matrix). All I wanted to do - is to run a different wasm app for each pixel, and it (kinda) works! I had to do a bit of optimizations, and reduced all stacks and memory limits to minimum, but I finally got all of 25 pixels running.

<blockquote class="twitter-tweet"><p lang="en" dir="ltr">Each pixel on this esp32 board is controlled by a different WebAssembly app. <a href="https://twitter.com/hashtag/arduino?src=hash&amp;ref_src=twsrc%5Etfw">#arduino</a> <a href="https://twitter.com/wasm3_engine?ref_src=twsrc%5Etfw">@wasm3_engine</a> <a href="https://t.co/ioZZJzUvAw">pic.twitter.com/ioZZJzUvAw</a></p>&mdash; zubr kabbi (@zubr_kabbi) <a href="https://twitter.com/zubr_kabbi/status/1436833749359017985?ref_src=twsrc%5Etfw">September 11, 2021</a></blockquote>

Using `wasm3` as wasm runtime. Apps are written in c (for now), see them in `./wasm` folder.

Try for yourself (you'll need `wasi-sdk` and `wasm-micro-runtime`)
```
export WAMR_DIR="path to wamr installation" ; that's where I've stolen the build script from
./build-apps.sh && ls wasm/*.wasm | xargs -n 1 xxd -i > src/data.h
pio run -t upload
```
