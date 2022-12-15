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

      # Install cmake v3.25.1 to $TOOL_ROOT/bin
      > $(basename $0) v3.25.1

   Repos:

      https://github.com/Kitware/CMake

EOF
}

# --------------------------------------------------------------------- valgrind

build_cmake()
{
    local VERSION="$1"
    local FILE_BASE="cmake-$VERSION"
    local FILE="$FILE_BASE.tar.gz"

    cd "$TMPD"
    wget "https://github.com/Kitware/CMake/archive/refs/tags/$VERSION.tar.gz"
    cat "$VERSION.tar.gz" | gzip -dc | tar -xf -

    cd "CMake-${VERSION:1}"
    ./configure --prefix="$TOOL_ROOT"
    nice ionice -c3 make -j$(nproc)
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
        ACTION="build_cmake ${ARG}" && continue
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

