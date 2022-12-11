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

build_doxygen()
{
    VERSION="$1"
    VERSION_TAG="$(echo "$VERSION" | sed 's,\.,_,g')"

    cd "$TMPD"
    git clone -b Release_${VERSION_TAG} https://github.com/doxygen/doxygen
    mkdir doxygen/build
    cd doxygen/build
    
    export CMAKE_PREFIX_PATH=$TOOL_ROOT
    $CMAKE -D CMAKE_BUILD_TYPE=Release             \
           -D english_only=ON                      \
           -D build_doc=OFF                        \
           -D build_wizard=ON                      \
           -D build_search=ON                      \
           -D build_xmlparser=ON                   \
           -D use_libclang=ON                      \
           -D CMAKE_INSTALL_PREFIX:PATH=$TOOL_ROOT \
           ..

    make -j
    make install
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
        ACTION="build_doxygen ${ARG}" && continue
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


