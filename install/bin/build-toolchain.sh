#!/bin/bash

set -e

source "$(cd "$(dirname "$0")" ; pwd)/env.sh"

show_help()
{
    cat <<EOF

   Usage: $(basename $0) OPTION* <tool>

   Option:

      --cleanup           Remove temporary files after building
      --no-cleanup        Do not remove temporary files after building

   Tool:

      gcc-x.y.z
      llvm-x.y.z

   Examples:

      # Install gcc version 12.2.0 to $OPT_ROOT
      > $(basename $0) gcc-12.2.0

      # Install clang version 15.0.6 to $OPT_ROOT
      > $(basename $0) clang-15.0.6

   Repos:

      https://github.com/gcc-mirror/gcc
      https://github.com/llvm/llvm-project

EOF
}

# ------------------------------------------------------------------------ clang

build_llvm()
{
    local CLANG_V="$1"
    local TAG="$2"
    local LLVM_DIR="llvm"

    local SRC_D="$TMPD/$LLVM_DIR"
    local BUILD_D="$TMPD/build-llvm-${TAG}"
    local INSTALL_PREFIX="${OPT_ROOT}/clang-${CLANG_V}"
    
    rm -rf "$BUILD_D"
    mkdir -p "$SRC_D"
    mkdir -p "$BUILD_D"

    cd "$SRC_D"

    ! [ -d "llvm-project" ] && git clone https://github.com/llvm/llvm-project.git
    cd llvm-project
    git checkout main
    git pull origin main
    git checkout "llvmorg-${CLANG_V}"

    cd "$BUILD_D"

    # NOTE, to build lldb, may need to specify the python3
    #       variables below, and something else for CURSES
    # -DPYTHON_EXECUTABLE=/usr/bin/python3.6m \
    # -DPYTHON_LIBRARY=/usr/lib/python3.6/config-3.6m-x86_64-linux-gnu/libpython3.6m.so \
    # -DPYTHON_INCLUDE_DIR=/usr/include/python3.6m \
    # -DCURSES_LIBRARY=/usr/lib/x86_64-linux-gnu/libncurses.a \
    # -DCURSES_INCLUDE_PATH=/usr/include/ \

    nice ionice -c3 cmake -G "Unix Makefiles" \
         -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;libcxx;libcxxabi;libunwind;compiler-rt;lld;polly" \
         -DCMAKE_BUILD_TYPE=release \
         -DCMAKE_C_COMPILER=$CC_COMPILER \
         -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
         -DLLVM_ENABLE_ASSERTIONS=Off \
         -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=Yes \
         -DLIBCXX_ENABLE_SHARED=YES \
         -DLIBCXX_ENABLE_STATIC=YES \
         -DLIBCXX_ENABLE_FILESYSTEM=YES \
         -DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=YES \
         -LLVM_ENABLE_LTO=thin \
         -LLVM_USE_LINKER=$LINKER \
         -DLLVM_BUILD_LLVM_DYLIB=YES \
         -DCURSES_LIBRARY=/usr/lib/x86_64-linux-gnu/libncurses.so \
         -DCURSES_INCLUDE_PATH=/usr/include/ \
         -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX} \
         $SRC_D/llvm-project/llvm

    $TIMECMD nice make -j$(nproc) 2>$BUILD_D/stderr.text | tee $BUILD_D/stdout.text
    sudo make install 2>>$BUILD_D/stderr.text | tee -a $BUILD_D/stdout.text
    cat $BUILD_D/stderr.text   
}

# -------------------------------------------------------------------------- gcc

build_gcc()
{
    local TAG="$1"
    local SUFFIX="$1"
    if [ "$2" != "" ] ; then SUFFIX="$2" ; fi
    
    local MAJOR_VERSION="$(echo "$SUFFIX" | sed 's,\..*$,,')"
    local SRCD="$TMPD/$SUFFIX"
    
    mkdir -p "$SRCD"
    cd "$SRCD"
    if [ ! -d "gcc" ] ;then
        git clone -b releases/gcc-${TAG} https://github.com/gcc-mirror/gcc.git
        cd gcc
        contrib/download_prerequisites
    fi

    if [ -d "$SRCD/build" ] ; then rm -rf "$SRCD/build" ; fi
    mkdir -p "$SRCD/build"
    cd "$SRCD/build"

    local PREFIX="${OPT_ROOT}/gcc-${SUFFIX}"
    
    nice ../gcc/configure --prefix=${PREFIX} \
         --enable-languages=c,c++,objc,obj-c++ \
         --disable-multilib \
         --program-suffix=-${MAJOR_VERSION} \
         --enable-checking=release \
         --with-gcc-major-version-only
    $TIMECMD nice make -j$(nproc) 2>$SRCD/build/stderr.text | tee $SRCD/build/stdout.text
    make install | tee -a $SRCD/build/stdout.text

    # Install symlinks to /usr/local
    ensure_link "$PREFIX/bin/gcc-${MAJOR_VERSION}"        /usr/local/bin/gcc-${MAJOR_VERSION}
    ensure_link "$PREFIX/bin/g++-${MAJOR_VERSION}"        /usr/local/bin/g++-${MAJOR_VERSION}
    ensure_link "$PREFIX/bin/gcov-${MAJOR_VERSION}"       /usr/local/bin/gcov-${MAJOR_VERSION}
    ensure_link "$PREFIX/bin/gcov-dump-${MAJOR_VERSION}"  /usr/local/bin/gcov-dump-${MAJOR_VERSION}
    ensure_link "$PREFIX/bin/gcov-tool-${MAJOR_VERSION}"  /usr/local/bin/gcov-tool-${MAJOR_VERSION}
    ensure_link "$PREFIX/bin/gcc-ranlib-${MAJOR_VERSION}" /usr/local/bin/gcc-ranlib-${MAJOR_VERSION}
    ensure_link "$PREFIX/bin/gcc-ar-${MAJOR_VERSION}"     /usr/local/bin/gcc-ar-${MAJOR_VERSION}
    ensure_link "$PREFIX/bin/gcc-nm-${MAJOR_VERSION}"     /usr/local/bin/gcc-nm-${MAJOR_VERSION}
    ensure_link "$PREFIX/include/c++/${MAJOR_VERSION}"    /usr/local/include/c++/${MAJOR_VERSION}
    ensure_link "$PREFIX/lib64"                           /usr/local/lib/gcc/${MAJOR_VERSION}
    ensure_link "$PREFIX/lib/gcc"                         /usr/local/lib/gcc/${MAJOR_VERSION}/gcc
}

# ------------------------------------------------------------------------ parse

if (( $# == 0 )) ; then
    show_help
    exit 0
fi

NO_CLEANUP=1
ACTION=""
while (( $# > 0 )) ; do
    ARG="$1"
    shift

    [ "$ARG" = "-h" ] || [ "$ARG" = "--help" ] && show_help && exit 0
    [ "$ARG" = "--cleanup" ] && NO_CLEANUP=0 && continue
    [ "$ARG" = "--no-cleanup" ] && NO_CLEANUP=1 && continue
    [ "${ARG:0:3}" = "gcc" ] && ACTION="build_gcc ${ARG:4}" && continue
    [ "${ARG:0:4}" = "llvm" ] && ACTION="build_llvm ${ARG:5}" && continue
    [ "${ARG:0:5}" = "clang" ] && ACTION="build_llvm ${ARG:6}" && continue

    echo "unexpected argument '$ARG'" 1>&2 && exit 1
done

# ---------------------------------------------------------------------- cleanup

cleanup()
{
    if [ "$CLEANUP" = "True" ] ; then
        rm -rf "$TMPD"
        rm -f "$HOME/user-config.jam"
    fi    
}

make_working_dir()
{
    if [ "$CLEANUP" = "True" ] ; then
        TMPD="$(mktemp -d /tmp/$(basename "$0").XXXXXX)"
    else
        TMPD="/tmp/${USER}-$(basename "$0")"
    fi
    if [ "$CLEANUP" = "False" ] ; then
        mkdir -p "$TMPD"
        echo "Working directory set to: TMPD=$TMPD"
    fi

    trap cleanup EXIT
}

# ----------------------------------------------------------------------- action

if [ "$ACTION" != "" ] ; then
    make_working_dir "$CLEANUP"
    ensure_directory $OPT_ROOT
    ensure_directory $ARCH_ROOT
    install_dependences
    $ACTION
fi

