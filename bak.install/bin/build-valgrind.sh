#!/bin/bash

set -e

source "$(cd "$(dirname "$0")" ; pwd)/env.sh"

show_help()
{
    cat <<EOF

   Usage: $(basename $0) OPTION* <version>

   Option:

      --cleanup           Remove temporary files after building
      --no-cleanup        Do not remove temporary files after building

   Examples:

      # Install valgrind 3.20 to $TOOL_ROOT/bin
      > $(basename $0) 3.20.0

   Repos:

      http://www.valgrind.org/downloads

EOF
}

# --------------------------------------------------------------------- valgrind

build_valgrind()
{
    VALGRIND_VERSION="$1"
    URL="https://sourceware.org/pub/valgrind/valgrind-${VALGRIND_VERSION}.tar.bz2"
    VAL_D="$TMPD/valgrind-${VALGRIND_VERSION}"
    rm -rf "$VAL_D"
    cd "$TMPD"
    wget "$URL"
    bzip2 -dc valgrind-${VALGRIND_VERSION}.tar.bz2 | tar -xf -
    rm -f valgrind-${VALGRIND_VERSION}.tar.bz2
    cd "$VAL_D"
    export CC=$CC_COMPILER
    export CXX=$CXX_COMPILER
    ./autogen.sh
    ./configure --prefix="$TOOL_ROOT"
    nice ionice -c3 make -j
    sudo make install
}

# ------------------------------------------------------------------------ parse

if (( $# == 0 )) ; then
    show_help
    exit 0
fi

CLEANUP="True"
ACTION=""
while (( $# > 0 )) ; do
    ARG="$1"
    shift

    [ "$ARG" = "-h" ] || [ "$ARG" = "--help" ] && show_help && exit 0
    [ "$ARG" = "--cleanup" ] && CLEANUP="True" && continue
    [ "$ARG" = "--no-cleanup" ] && CLEANUP="False" && continue

    if [ "$ACTION" = "" ] ; then
        ACTION="build_valgrind ${ARG}" && continue
    fi
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
    ensure_directory "$TOOL_ROOT"
    install_dependences
    $ACTION
fi

