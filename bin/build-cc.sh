#!/bin/bash

set -e

# sudo echo "Got root permissions"

OPT_ROOT=/opt/cc
ARCH_ROOT=/opt/arch

if [ "$(uname)" = "Darwin" ] ; then
    OSX=1
    NPROC=$(sysctl -n hw.ncpu)
    TIMECMD="gtime -v"
else
    OSX=0
    NPROC=$(nproc)
    TIMECMD="/usr/bin/time -v"
fi

# -------------------------------------------------- ensure subversion installed

sudo apt-get install -y \
     wget subversion automake swig python2.7-dev libedit-dev libncurses5-dev  \
     python3-dev python3-pip python3-tk python3-lxml python3-six              \
     libparted-dev flex sphinx-doc guile-2.2 gperf gettext expect tcl dejagnu \
     libgmp-dev libmpfr-dev libmpc-dev libasan6 lld-10 clang-10

# ------------------------------------------------------------------ environment

CC_COMPILER=clang-10
CXX_COMPILER=clang++-10
LINKER=lld-10
PYTHON_VERSION="$(python3 --version | awk '{print $2}' | sed 's,.[0-9]$,,')"

# ------------------------------------------------------------------------ boost

build_boost()
{
    CXX="$1"
    VERSION="$2"
    PREFIX="$3"
    TOOLSET="$4"
    STD="$5"

    BOOST_VERSION=1_76_0
    NO_CLEANUP=1

    if [ "$NO_CLEANUP" = "1" ] ; then
        TMPD=$HOME/TMP/boost
        mkdir -p $TMPD
    else
        TMPD=$(mktemp -d /tmp/$(basename $0).XXXXXX)
    fi

    cd $TMPD
    FILE=boost_${BOOST_VERSION}.tar.gz
    [ ! -f "$FILE" ] \
        && wget    https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/$(echo $BOOST_VERSION | sed 's,_,.,g')/source/boost_${BOOST_VERSION}.tar.bz2

    [ -d "boost_${BOOST_VERSION}" ] && rm -rf boost_${BOOST_VERSION}

    tar -xf boost_${BOOST_VERSION}.tar.bz2
    cd boost_${BOOST_VERSION}
    cp tools/build/example/user-config.jam $HOME

    CPPFLAGS=""
    LDFLAGS=""
    
    cat >> $HOME/user-config.jam <<EOF
using python 
   : 3.6 
   : /usr/bin/python3 
   : /usr/include/python3.6m 
   : /usr/lib 
   ;

using $TOOLSET
   : $VERSION
   : $CXX 
   : <cxxflags>"$CPPFLAGS" 
     <linkflags>"$LDFLAGS" 
   ;
EOF

    #  --with-libraries=filesystem,system,python,graph
    ./bootstrap.sh --prefix=$PREFIX
    ./b2 -j $(nproc) toolset=$TOOLSET cxxstd=$STD
    sudo ./b2 install toolset=$TOOLSET cxxstd=$STD
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

    $TIMECMD nice make -j$NPROC 2>$BUILD_D/stderr.text | tee $BUILD_D/stdout.text
    sudo make install 2>>$BUILD_D/stderr.text | tee -a $BUILD_D/stdout.text
    cat $BUILD_D/stderr.text   
}

# -------------------------------------------------------------------------- gcc

ensure_link()
{
    local SOURCE="$1"
    local DEST="$2"

    if [ ! -e "$SOURCE" ] ; then echo "File/directory not found '$SOURCE'" ; exit 1 ; fi
    sudo mkdir -p "$(dirname "$DEST")"
    sudo rm -f "$DEST"
    sudo ln -s "$SOURCE" "$DEST"
}

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
         --enable-languages=c,c++,objc,obj-c++,fortran,d \
         --disable-multilib \
         --program-suffix=-${MAJOR_VERSION} \
         --enable-checking=release \
         --with-gcc-major-version-only
    $TIMECMD nice make -j$NPROC 2>$SRCD/build/stderr.text | tee $SRCD/build/stdout.text
    make install | tee -a $SRCD/build/stdout.text

    if [ "$OSX" = "0" ] ; then
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
    fi
}

# ------------------------------------------------------------------------- wasm

# build_wasm()
# {
#     [ -d ${OPT_ROOT}/WASM ] && sudo rm -rf ${OPT_ROOT}/WASM
#     sudo mkdir -p ${OPT_ROOT}/WASM
#     sudo chown -R amichaux:amichaux ${OPT_ROOT}/WASM

#     cd ${OPT_ROOT}/WASM
#     git clone https://github.com/juj/emsdk.git
#     cd emsdk
#     ./emsdk install  --build=Release sdk-incoming-64bit binaryen-master-64bit
#     ./emsdk activate --build=Release sdk-incoming-64bit binaryen-master-64bit
#     source ./emsdk_env.sh --build=Release
# }

# --------------------------------------------------------------------- valgrind

build_valgrind()
{
    VALGRIND_VERSION="$1"
    URL="http://www.valgrind.org/downloads/valgrind-${VALGRIND_VERSION}.tar.bz2"
    VAL_D="$TMPD/valgrind-${VALGRIND_VERSION}"
    rm -rf "$VAL_D"
    cd "$TMPD"
    wget "$URL"
    bzip2 -dc valgrind-${VALGRIND_VERSION}.tar.bz2 | tar -xf -
    rm -f valgrind-${VALGRIND_VERSION}.tar.bz2
    cd "$VAL_D"
    export CC=$CC_COMPILER
    ./autogen.sh
    ./configure --prefix=/usr/local
    nice ionice -c3 make -j$(nproc)
    sudo make install
}

# ------------------------------------------------------------------------ build

show_help()
{
    cat <<EOF

   Usage: $(basename $0) OPTION* <tool>

   Option:

      --cleanup           Remove temporary files after building
      --no-cleanup        Do not remove temporary files after building

   Tool:

      gcc-x.y.z
      valgrind-x.y.z
      llvm-x.y.z

EOF
}

NO_CLEANUP=1
ACTION=""
while (( $# > 0 )) ; do
    ARG="$1"
    shift

    [ "$ARG" = "-h" ] || [ "$ARG" = "--help" ] && show_help && exit 0
    [ "$ARG" = "--cleanup" ] && NO_CLEANUP=0
    [ "$ARG" = "--no-cleanup" ] && NO_CLEANUP=1
    [ "${ARG:0:3}" = "gcc" ] && ACTION="build_gcc ${ARG:4}"
    [ "${ARG:0:4}" = "llvm" ] && ACTION="build_llvm ${ARG:5}"
done

if [ "$NO_CLEANUP" = "1" ] ; then
    TMPD=/tmp/${USER}-build-cc    
else
    TMPD=$(mktemp -d /tmp/$(basename $0).XXXXXX)
fi

trap cleanup EXIT
cleanup()
{
    if [ "$NO_CLEANUP" != "1" ] ; then
        rm -rf $TMPD
        rm -f $HOME/user-config.jam
    fi    
}

ensure_directory()
{
    local D="$1"
    if [ ! -d "$D" ] ; then
        echo "Directory '$D' does not exist, creating..."
        sudo mkdir -p "$D"
    fi
    if [ ! -w "$D" ] ; then
        echo "Directory '$D' is not writable by $USER, chgrp..."
        sudo chgrp -R staff "$D"
        sudo chgrp -R adm "$D"
        sudo chmod 775 "$D"
    fi
    if [ ! -d "$D" ] || [ ! -w "$D" ] ; then
        echo "Failed to create writable directory '$D', aborting"
        exit 1
    fi
}

if [ "$ACTION" != "" ] ; then
    if [ "$NO_CLEANUP" = "1" ] ; then
        mkdir -p $TMPD
        echo "No-cleanup set: TMPD=$TMPD"
    fi
    ensure_directory $OPT_ROOT
    ensure_directory $ARCH_ROOT
    $ACTION
fi

#build_clang llvmorg-13-init    13.0.0
#build_clang llvmorg-12.0.0     12.0.0
#build_clang llvmorg-11.1.0     11.1.0
#build_clang llvmorg-11.0.1     11.0.1
#build_gcc 11.2.0
#build_valgrind 3.14.0

#build_boost 
