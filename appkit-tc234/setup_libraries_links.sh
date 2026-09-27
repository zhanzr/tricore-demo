#!/usr/bin/env bash
# Make the shared board-level Libraries/ visible inside each project, for
# tools that expect Libraries inside the project folder (e.g. the AURIX Studio
# IDE, whose .cproject uses ${ProjDirPath}/Libraries).
#
# The CLI build scripts reference ../Libraries directly and do NOT need this.
# Re-run after a fresh clone. Symlinks/junctions are not tracked by git.
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

if [[ ! -d "$target" ]]; then
    echo "ERROR: shared Libraries folder not found: $target" >&2
    exit 1
fi

projects=(bare/blink_hello bare/dhry_200m bare/coremark_200m bare/pwm_buzz_test)

winify() {
    # POSIX path -> Windows path (cygpath is present in MSYS2 and Git Bash)
    cygpath -w "$1" 2>/dev/null || echo "$1"
}

for p in "${projects[@]}"; do
    link="$board/$p/Libraries"
    if [[ -e "$link" || -L "$link" ]]; then
        echo "Exists (leave as-is): $link"
        continue
    fi

    # 1) try a native symlink
    if ln -s "$target" "$link" 2>/dev/null && [[ -L "$link" ]]; then
        echo "Linked (symlink): $link -> $target"
        continue
    fi
    rm -rf "$link"   # remove the plain copy ln may have left behind

    # 2) fall back to a Windows directory junction (no admin required)
    if command -v cmd >/dev/null 2>&1 && command -v cygpath >/dev/null 2>&1; then
        if cmd //c mklink //J "$(winify "$link")" "$(winify "$target")" >/dev/null 2>&1 \
           && [[ -e "$link" ]]; then
            echo "Linked (junction): $link -> $target"
            continue
        fi
    fi

    echo "FAILED: $link"
done
