
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

### Technologies To Learn
 * P2300, senders/receivers
 * Ranges
 * Modules (when clang-scan-deps is available)
 * Concepts
 * source_location
 * Calendar/Time Zone library
 * jthread
 * owned<T> (dumb) smart pointer (library)

#### Build System

 * cmake (groan)
 * LTO
 * unity build
 * asan/usan/tsan/debug/release
 * testing with gtest/catch
 * constexpr-testing
 * fuzz testing
 * coverage
 * benchmark
 * compiled examples
 * doxygen

#### Subsystems

 * refined "niggly" library. (Rename?)
   - Make every string function aware of custom string types (ranges)
   - Polyfill methods
 * websockets -- websockets to work in WebAssembly (through Javascript), and stand alone executable
 * Symmetric client/server rpc service over websocket layer
 * cpp_asio_grpc_unifex, a grpc example
 * StateMachine, with undo/redo checkpoints (external polymorphism?)
 * User Interface
   - SDL/ImGUI of Webassembly
   - Curses server with logging to file (tail -f) Always wanted to do this
 
 



