
# ------------------------------------------------------ Host Platform Variables
export TOOL_ROOT=/opt/tools
export OPT_ROOT=/opt/tools/toolchains
export ARCH_ROOT=/opt/arch

# Tool (host) compilers
export CC_COMPILER=clang-10
export CXX_COMPILER=clang++-10
export LINKER=lld-10

export CC_COMPILER=/usr/bin/gcc
export CXX_COMPILER=/usr/bin/g++
export LINKER=/usr/bin/ld


export PYTHON_VERSION="$(python3 --version | awk '{print $2}' | sed 's,.[0-9]$,,')"
export TIMECMD="/usr/bin/time -v"

export CLEANUP="True"

export CMAKE="$TOOL_ROOT/bin/cmake"

# --------------------------------------------------------------------- Platform
export IS_UBUNTU=$([ -x /usr/bin/lsb_release ] && lsb_release -a 2>/dev/null | grep -q Ubuntu && echo 1 || echo 0)
export IS_FEDORA=$([ -f /etc/fedora-release ] && echo 1 || echo 0)


ensure_link()
{
    local SOURCE="$1"
    local DEST="$2"

    if [ ! -e "$SOURCE" ] ; then echo "File/directory not found '$SOURCE'" ; exit 1 ; fi
    sudo mkdir -p "$(dirname "$DEST")"
    sudo rm -f "$DEST"
    sudo ln -s "$SOURCE" "$DEST"
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

install_dependences()
{
    # If compiling for a different platforms, we'd augment this files with
    # brew commands (macos), yum (fedora) etc.
    export DEBIAN_FRONTEND=noninteractive
    sudo apt-get install -y \
         wget subversion automake swig python2.7-dev libedit-dev libncurses5-dev  \
         python3-dev python3-pip python3-tk python3-lxml python3-six              \
         libparted-dev flex sphinx-doc guile-2.2 gperf gettext expect tcl dejagnu \
         libgmp-dev libmpfr-dev libmpc-dev libasan6 lld-10 clang-10
}

cross_env_setup()
{
    local TOOLCHAIN="$1"

    [ "${TOOLCHAIN:0:3}" = "gcc" ] && export IS_GCC="True"   || export IS_GCC="False"
    [ "$IS_GCC" = "False" ]        && export IS_CLANG="True" || export IS_CLANG="False"

    export TOOLCHAIN_ROOT="$OPT_ROOT/$TOOLCHAIN"
    export PREFIX="$ARCH_ROOT/$TOOLCHAIN"
    export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"

    if [ "$IS_GCC" = "True" ] ; then
        export TOOLCHAIN_NAME="gcc"
        export CC="$TOOLCHAIN_ROOT/bin/gcc-12"
        export CXX="$TOOLCHAIN_ROOT/bin/g++-12"
        export AR="$TOOLCHAIN_ROOT/bin/gcc-ar-12"
        export NM="$TOOLCHAIN_ROOT/bin/gcc-nm-12"
        export RANLIB="$TOOLCHAIN_ROOT/bin/gcc-ranlib-12"
        export GCOV="$TOOLCHAIN_ROOT/bin/gcov-12"
    elif [ "$IS_CLANG" = "True" ] ; then
        export TOOLCHAIN_NAME="clang"
        export CC="$TOOLCHAIN_ROOT/bin/clang"
        export CXX="$TOOLCHAIN_ROOT/bin/clang++"
        export AR="$TOOLCHAIN_ROOT/bin/llvm-ar"
        export NM="$TOOLCHAIN_ROOT/bin/llvm-nm"
        export RANLIB="$TOOLCHAIN_ROOT/bin/llvm-ranlib"
        export LLD="$TOOLCHAIN_ROOT/bin/ld.lld"
    else
        echo "logic error" 1>&2 && exit 1
    fi

    export CXXSTD="-std=c++20"
    export CFLAGS="-fPIC -O3"
    export CXXFLAGS="-fPIC -O3"
    [ "$IS_CLANG" = "True" ] && LDFLAGS="-fuse-ld=$LLD" || LD_FLAGS=""
    LDFLAGS+="-L$PREFIX/lib -Wl,-rpath,$PREFIX/lib"
    export LDFLAGS="$LDFLAGS"
    export LIBS="-lm -pthreads"
}

print_env()
{
    cat <<EOF

    TOOL_ROOT:       $TOOL_ROOT  
    ARCH_ROOT:       $ARCH

    IS_UBUNTU:       $IS_UBUNTU
    IS_GCC:          $IS_GCC
    IS_CLANG:        $IS_CLANG

    PREFIX:          $PREFIX
    TOOLCHAIN_ROOT:  $TOOLCHAIN_ROOT  
    PKG_CONFIG_PATH: $PKG_CONFIG_PATH

    CC:              $CC
    CXX:             $CXX
    AR:              $AR
    NM:              $NM
    RANLIB:          $RANLIB
    GCOV:            $GCOV
    LLD:             $LLD

    CXXSTD:          $CXXSTD
    CFLAGS:          $CFLAGS
    CXXFLAGS:        $CXXFLAGS

    LDFLAGS:         $LDFLAGS
    LIBS:            $LIBS

EOF
}



