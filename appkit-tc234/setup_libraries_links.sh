#!/usr/bin/env bash
# Make the shared board-level Libraries/ visible inside each project, for
# tools that expect Libraries inside the project folder (e.g. the AURIX Studio
# IDE, whose .cproject uses ${ProjDirPath}/Libraries).
#
# The CLI build scripts reference ../Libraries directly and do NOT need this.
# Re-run after a fresh clone. Symlinks/junctions are not tracked by git.
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

projects=(blink_hello dhry_200m coremark_200m pwm_buzz_test)

for p in "${projects[@]}"; do
    link="$board/$p/Libraries"
    if [[ -e "$link" || -L "$link" ]]; then
        echo "Exists (leave as-is): $link"
        continue
    fi
    ln -s "$target" "$link"
    echo "Linked: $link -> $target"
done
