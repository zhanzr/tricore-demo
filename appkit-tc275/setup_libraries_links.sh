#!/usr/bin/env bash
# Recreate per-project Libraries links pointing to the shared board-level
# Libraries folder. Only needed if you open the projects in the AURIX
# Studio IDE (its .cproject references ${ProjDirPath}/Libraries). The CLI
# build scripts reference the shared folder directly and do NOT need
# these links.
#
# On POSIX systems a symlink is created. On Windows (MSYS2 / Git Bash)
# `ln -s` either makes a plain copy or requires admin rights, so the
# script falls back to a directory junction via `cmd /c mklink /J`
# (junctions need no admin and work for the ADS IDE).
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

winify() {
    # POSIX path -> Windows path (cygpath is present in MSYS2 and Git Bash)
    cygpath -w "$1" 2>/dev/null || echo "$1"
}

for p in bare/blink_hello bare/coremark_200m bare/dhry_200m bare/st7789s_md120_240x240_ft6336; do
    link="$board/$p/Libraries"
    if [ -e "$link" ] || [ -L "$link" ]; then
        echo "Exists (leave as-is): $link"
        continue
    fi

    # 1) try a native symlink
    if ln -s "$target" "$link" 2>/dev/null && [ -L "$link" ]; then
        echo "Linked (symlink): $link -> $target"
        continue
    fi
    rm -rf "$link"   # remove the plain copy ln may have left behind

    # 2) fall back to a Windows directory junction (no admin required)
    if command -v cmd >/dev/null 2>&1 && command -v cygpath >/dev/null 2>&1; then
        if cmd //c mklink //J "$(winify "$link")" "$(winify "$target")" >/dev/null 2>&1 \
           && [ -e "$link" ]; then
            echo "Linked (junction): $link -> $target"
            continue
        fi
    fi

    echo "FAILED: $link"
done
