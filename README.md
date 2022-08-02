
# zero

### About

This repo is a playground where I teach myself new stuff about C++.

### Dependencies

 * https://github.com/axboe/liburing
 * https://github.com/facebookexperimental/libunifex.git
 
```
git clone https://github.com/facebookexperimental/libunifex.git
cd libunifex
cmake -H. -Bbuild -DCMAKE_CXX_STANDARD:STRING=20 -DCMAKE_INSTALL_PREFIX=/usr/local
cd build
make -j
sudo make install
```

### Building

Since this is a personal project, it very much is "works for me". I use Ubuntu and MacOS.

Use the build script `bin/build-cc.sh` to install custom versions of clang (llvm) and gcc.

The `./run.sh` script is a front end to the Makefile... it:
 * Sets environment variables that change the mode of the Makefile.
 * Runs make
 * Runs the code, including under gdb.

### Plan

#### Build System

 * LTO
 * unity build
 * asan/usan/tsan/debug/release
 * testing/coverage
 * constexpr-testing
 * benchmark (?)

#### Subsystems

 * websockets -- websockets to work in WebAssembly
 * Client/server rpc service with push
 * SDL/ImGUI (?) Do I learn Qt6 (?)
 * scheduler to do co-routines
 * test timers on threadpool
 * use ranges with string methods
 * Make every string function aware of custom string types (ranges)
 * Statemachine, with undo/redo checkpoints (external polymorphism?)
 * Curses server with logging to file (tail -f) Always wanted to do this
 
 



