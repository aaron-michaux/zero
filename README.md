
# zero

This is all quite broken at the moment. I wrote my own implementation of C++ futures, and then realized that I really need to learn P2300 senders+receivers.

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

###
 * Spin out install stuff, including `project-config`

### Technologies To Learn
 * P2300, senders/receivers
   - With Linux's io_uring
 * Ranges
 * Modules (when clang-scan-deps is available)
 * Concepts
 * source_location
 * Calendar/Time Zone library
 * jthread
 * owned<T> (dumb) smart pointer (library)
 * coroutines

#### Build System

 * arch directory for dependencies
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
 * cmake? (groan)

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
 
#### Cyber-Security executive order
 * Herb Sutter https://youtu.be/ELeZAKCN4tY?t=2439
 * Executive order on cyber-security, May/8th/2021 https://www.whitehouse.gov/briefing-room/presidential-actions/2021/05/12/executive-order-on-improving-the-nations-cybersecurity/
 * Dept of Commerce and NiST (national institute standard of technology) charged with asking how to make software safer:
   https://arxiv.org/pdf/2107.12850
 * US government funding projects to rewrite C/C++ code
   - A VM for LLVM bitcode? (Sulong?)
   - Cppfront/Carbon are "pre-alpha"

