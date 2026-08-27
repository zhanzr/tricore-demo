#!/usr/bin/env bash
# Recreate per-project Libraries symlinks pointing to the shared board-level
# Libraries folder. Only needed if you open the projects in the AURIX Studio IDE
# (its .cproject references ${ProjDirPath}/Libraries). The CLI build scripts
# reference the shared folder directly and do NOT need these links.
#
# Run from this board folder:
#   bash setup_libraries_links.sh

set -euo pipefail

board="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
target="$board/Libraries"

if [ ! -d "$target" ]; then
    echo "Shared Libraries folder not found: $target" >&2
    exit 1
fi

for p in blink_hello coremark dhry; do
    link="$board/$p/Libraries"
    if [ -e "$link" ]; then
        echo "Exists (leave as-is): $link"
        continue
    fi
    ln -s "$target" "$link"
    if [ -e "$link" ]; then
        echo "Linked: $link -> $target"
    else
        echo "FAILED: $link"
    fi
done
