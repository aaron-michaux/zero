#!/bin/bash

set -e

PPWD="$(cd "$(dirname "$0")"; pwd)"

# ------------------------------------------------------------ Parse Commandline

CONFIG=asan
TARGET_FILE0=zero
TOOLCHAIN=gcc-11
FEEDBACK=0
NO_BUILD=0
GDB=0
LLDB=0
BUILD_ONLY=0
BUILD_TESTS=0
BENCHMARK=0
BUILD_EXAMPLES=0
LTO=0
UNITY_BUILD=0
VALGRIND=0
HELGRIND=0
PYTHON_BINDINGS=0
COVERAGE=0
COVERAGE_HTML=0
RULE=all
CXXSTD=-std=c++2b
LOG_LEVEL=""

TARGET_FILE="$TARGET_FILE0"

show_usage()
{
    cat <<EOF

   $(basename $0) [OPTIONS...]* [-- other arguments]?

   Compiler options:
      clang, clang-14, gcc, gcc-9, gcc-11 (default)

   Configuration options:
      asan (default), usan, tsan, debug, release, reldbg, valgrind, helgrind gdb, lldb
      
      If gdb, lldb, or valgrind is selected, then builds debug and 
      runs under the tool.

   Other options:
      clean         ie, make clean for the configuration
      info          print out important environment variables for the build
      verbose       verbose output
      quiet         no output
      unity         do a unity build
      lto           enable lto
      no-lto        disable lto
      build         build but do not run
      example       build the examples
      test          build and run test cases
      coverage      build and run test cases with text code-coverage output
      coveragehtml  build and run test cases with html code-coverage output

   Examples:

      # Build and run the testcases in tsan mode
      > $(basename $0) tsan test

      # Build and run under gdb
      > $(basename $0) gdb

      # Make a unity build in release mode
      > $(basename $0) unity release

      # Make html test coverage using clang-14, passing arguments "1" "2" "3" to the executable
      > $(basename $0) clang-14 coveragehtml -- 1 2 3

EOF
}

while [ "$#" -gt "0" ] ; do
    
    # Compiler
    [ "$1" = "clang" ]     && TOOLCHAIN="clang-14" && shift && continue
    [ "$1" = "clang-14" ]  && TOOLCHAIN="clang-14" && shift && continue
    [ "$1" = "gcc" ]       && TOOLCHAIN="gcc-11"  && shift && continue
    [ "$1" = "gcc-9" ]     && TOOLCHAIN="gcc-9"  && shift && continue
    [ "$1" = "gcc-11" ]    && TOOLCHAIN="gcc-11" && shift && continue

    # Configuration
    [ "$1" = "asan" ]      && CONFIG=asan      && shift && continue
    [ "$1" = "usan" ]      && CONFIG=usan      && shift && continue
    [ "$1" = "tsan" ]      && CONFIG=tsan      && shift && continue
    [ "$1" = "debug" ]     && CONFIG=debug     && shift && continue
    [ "$1" = "reldbg" ]    && CONFIG=reldbg    && shift && continue
    [ "$1" = "release" ]   && CONFIG=release   && shift && continue
    [ "$1" = "valgrind" ]  && CONFIG=debug     && VALGRIND=1 && shift && continue
    [ "$1" = "helgrind" ]  && CONFIG=debug     && HELGRIND=1 && shift && continue
    [ "$1" = "gdb" ]       && CONFIG=debug     && GDB=1 && shift && continue
    [ "$1" = "lldb" ]      && CONFIG=debug     && LLDB=1 && shift && continue
    
    # Other options
    [ "$1" = "clean" ]     && RULE="clean"     && shift && continue
    [ "$1" = "info" ]      && RULE="info"      && shift && continue
    [ "$1" = "verbose" ]   && FEEDBACK="1"     && shift && continue
    [ "$1" = "quiet" ]     && FEEDBACK="0"     && shift && continue
    [ "$1" = "unity" ]     && UNITY_BUILD="1"  && shift && continue
    [ "$1" = "lto" ]       && LTO="1"          && shift && continue
    [ "$1" = "no-lto" ]    && LTO="0"          && shift && continue
    [ "$1" = "build" ]     && BUILD_ONLY="1"   && shift && continue    
    [ "$1" = "test" ]      && BUILD_TESTS="1"  && BUILD_EXAMPLES=1 && shift && continue
    [ "$1" = "bench" ]     && BENCHMARK=1      && shift && continue
    [ "$1" = "examples" ]  && BUILD_EXAMPLES=1 && shift && continue
    [ "$1" = "coverage" ]  && RULE="coverage"  \
        && BUILD_TESTS=1 && COVERAGE=1 && CONFIG=debug && shift && continue
    [ "$1" = "coveragehtml" ]  && RULE="coverage_html"  \
        && BUILD_TESTS=1 && COVERAGE_HTML=1 && CONFIG=debug && shift && continue

    [ "$1" = "-h" ] || [ "$1" = "--help" ] && show_usage && exit 0
    
    [ "$1" = "--" ]        && break
    
    break
done

if [ "$TOOLCHAIN" = "emcc" ] ; then
    echo "emcc not supported... waiting for modules support" 1>&2
    exit 1
elif [ "${TOOLCHAIN:0:5}" = "clang" ] ; then
    export TOOL=clang
    export TOOL_VERSION="${TOOLCHAIN:6}"
elif [ "${TOOLCHAIN:0:3}" = "gcc" ] ; then
    export TOOL=gcc
    export TOOL_VERSION="${TOOLCHAIN:4}"
fi

if [ "$BENCHMARK" = "1" ] && [ "$BUILD_TESTS" = "1" ] ; then
    echo "Cannot benchmark and build tests at the same time."
    exit 1
fi

# ---------------------------------------------------------------------- Execute

UNIQUE_DIR="${TOOLCHAIN}-${CONFIG}"
[ "$BUILD_TESTS" = "1" ] && UNIQUE_DIR="test-${UNIQUE_DIR}"
[ "$LTO" = "1" ]         && UNIQUE_DIR="${UNIQUE_DIR}-lto"
[ "$BENCHMARK" = "1" ]   && UNIQUE_DIR="bench-${UNIQUE_DIR}"
[ "$COVERAGE" = "1" ] || [ "$COVERAGE_HTML" = "1" ] && UNIQUE_DIR="coverage-${UNIQUE_DIR}"

export BUILDDIR="/tmp/build-${USER}/${UNIQUE_DIR}/${TARGET_FILE}"
export TARGETDIR="build/${UNIQUE_DIR}"

export TARGET="${TARGET_FILE}"
export TOOLCHAIN_NAME="${TOOLCHAIN}"
export TOOLCHAIN_CONFIG="${CONFIG}"
export STATIC_LIBCPP="0"
export VERBOSE="${FEEDBACK}"
export LTO="${LTO}"
export UNITY_BUILD="${UNITY_BUILD}"
export CXXSTD="${CXXSTD}"

export BUILD_TESTS="${BUILD_TESTS}"
export BUILD_EXAMPLES="${BUILD_EXAMPLES}"
export BENCHMARK="${BENCHMARK}"

if [ "$COVERAGE" = "1" ] || [ "$COVERAGE_HTML" = "1" ] ; then
    if [ "$TOOL" != "gcc" ] && [ "$COVERAGE_HTML" = "0" ]; then        
        echo "coverage only supported with gcc (TOOL=$TOOL)" 1>&2
        exit 1
    fi

    if [ "$TOOL" = "gcc" ] ; then
        COVERAGE_FLAGS="--coverage -fno-elide-constructors -fno-default-inline -fno-inline"
        COVERAGE_LINK="--coverage"
    else
        # https://gitlab.com/stone.code/scov
        COVERAGE_FLAGS="-fprofile-instr-generate -fcoverage-mapping -fno-elide-constructors -fno-inline"
        COVERAGE_LINK="-fprofile-instr-generate"
        RULE="llvm_coverage_html"
    fi
    export CFLAGS="$CXXFLAGS $COVERAGE_FLAGS"
    export CXXFLAGS="$CXXFLAGS $COVERAGE_FLAGS"
    export LDFLAGS="$CXXFLAGS $COVERAGE_LINK"
fi

export SRC_DIRECTORIES="src"

# export VERSION_HASH="$(git log | grep commit | head -n 1 | awk '{ print $2 }')"
# export VERSION_CRC32="$(echo -n $VERSION_HASH | gzip -c | tail -c8 | hexdump -n4 -e '"%u"')"
# export VERSION_DEFINES="-DVERSION_HASH=\"\\\"$VERSION_HASH\"\\\""
# export CPPFLAGS="$CPPFLAGS $VERSION_DEFINES"

[ "$TARGET_FILE" = "build.ninja" ] && exit 0

[ "$(uname)" = "Darwin" ] && NPROC="$(sysctl -n hw.ncpu)" || NPROC="$(nproc)"

do_make()
{
    make -j $NPROC $RULE
    RET="$?"
    if [ "$RET" != "0" ] ; then exit $RET ; fi
}
do_make

[ "$RULE" = "clean" ] && exit 0 || true
[ "$RULE" = "info" ] && exit 0 || true
[ "$BUILD_ONLY" = "1" ] && exit 0 || true
[ "$COVERAGE" = "1" ] && exit 0 || true
[ "$COVERAGE_HTML" = "1" ] && exit 0 || true

# ---- If we're building the executable (TARGET_FILE0), then run it

if [ "$TARGET_FILE" = "$TARGET_FILE0" ] ; then

    export LSAN_OPTIONS="suppressions=$PPWD/project-config/lsan.supp"
    export ASAN_OPTIONS="protect_shadow_gap=0,detect_leaks=0"

    export TF_CPP_MIN_LOG_LEVEL="1"
    export AUTOGRAPH_VERBOSITY="1"

    if [ "$CONFIG" = "asan" ] ; then
        export MallocNanoZone=0
    fi
    PRODUCT="$TARGETDIR/$TARGET_FILE"

    RET=0    
    if [ "$VALGRIND" = "1" ] ; then        
        valgrind --demangle=yes --tool=memcheck --leak-check=full --track-origins=yes --verbose --log-file=valgrind.log --gen-suppressions=all --suppressions=project-config/valgrind.supp "$PRODUCT" "$@"
        RET=$?
    elif [ "$HELGRIND" = "1" ] ; then        
        valgrind --demangle=yes --tool=helgrind --verbose --log-file=helgrind.log --gen-suppressions=all --suppressions=project-config/helgrind.supp "$PRODUCT" "$@"
        RET=$?
    elif [ "$GDB" = "1" ] ; then        
        gdb -x project-config/gdbinit -silent -return-child-result -statistics --args "$PRODUCT" "$@"
        RET=$?
    elif [ "$GDB" = "1" ] ; then        
        lldb -ex run -silent -return-child-result -statistics --args "$PRODUCT" "$@"
        RET=$?
    else
        "$PRODUCT" "$@"
        RET=$?        
    fi

    exit $RET

fi

