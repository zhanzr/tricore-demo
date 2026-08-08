#!/usr/bin/env bash
# Build dhry_200m for TC234 (TC23A) with the AURIX GCC toolchain, standalone.
# Optimized for maximum speed: -O3 with aggressive speed flags.
# Produces dhry_200m.hex for flashing via AURIXFlasher.
#
# Requirements:
#   - AURIX GCC toolchain (tricore-elf-gcc) on PATH, or set TRICORE_GCC to the
#     full compiler path (without the .exe suffix on Windows).
#   - The GCC linker script Lcf_Gnuc_Tricore_Tc.lsl in the project root.

set -euo pipefail

proj="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
libs="$(dirname "$proj")/Libraries"           # shared at board root
if [[ -n "${BUILD_DIR:-}" ]]; then
    build_dir="$BUILD_DIR/dhry_200m_build"
elif [[ -d /tmp && -w /tmp ]]; then
    build_dir="/tmp/dhry_200m_build"
else
    build_dir="$HOME/.cache/dhry_200m_build"
fi

gcc_bin="${TRICORE_GCC:-tricore-elf-gcc}"
objcopy="${gcc_bin%-gcc}-objcopy"
size="${gcc_bin%-gcc}-size"

rm -rf "$build_dir"
mkdir -p "$build_dir"

# --- Gather include paths from the .cproject (values are HTML-escaped) ---
mapfile -t include_dirs < <(
    sed -n 's/.*${ProjDirPath}\([^&]*\).*/\1/p' "$proj/.cproject" |
        sed '/\.opt/d; /^\/Configurations\/Debug$/d' |
        sort -u |
        while IFS= read -r p; do
            case "$p" in
                /Libraries*) printf '%s\n' "-I${libs}${p#/Libraries}" ;;
                *)           printf '%s\n' "-I${proj}${p}" ;;
            esac
        done
)

# --- Compile flags: maximum speed ---
# -O3 base; additional flags aggressively favor speed.
# NOTE: -ffast-math changes float semantics; acceptable for this benchmark.
common_flags=(
    -c -mcpu=tc23xx -D__HIGHTEC__ -D__TRICORE__
    -std=c11 -O3 -ffast-math -funroll-loops -finline-functions -fno-math-errno -g
    "${include_dirs[@]}"
)

# --- Compile all sources (shared Libraries + project root) ---
objs=()
while IFS= read -r src; do
    src="${src//\\//}"
    if [[ "$src" == "$libs"/* ]]; then
        rel="${src#$libs/}"
    else
        rel="${src#$proj/}"
    fi
    rel="${rel//\//_}"
    obj="$build_dir/${rel%.c}.o"
    mkdir -p "$(dirname "$obj")"
    echo "CC  $(basename "$src")"
    "$gcc_bin" "${common_flags[@]}" -o "$obj" "$src"
    objs+=("$obj")
done < <(find "$libs" "$proj" -maxdepth 10 -name '*.c' | sort)

# --- Link (libgcc after libc resolves soft-float double helpers) ---
echo "Linking..."
elf="$build_dir/dhry_200m.elf"
"$gcc_bin" -mcpu=tc23xx -T "$proj/Lcf_Gnuc_Tricore_Tc.lsl" -nostdlib \
    -Wl,--gc-sections -Wl,-Map="$build_dir/dhry_200m.map" \
    -o "$elf" "${objs[@]}" -lgcc -lc -lnosys -lgcc

# --- Hex ---
"$objcopy" -O ihex "$elf" "$build_dir/dhry_200m.hex"
echo "HEX: $build_dir/dhry_200m.hex"
echo "SIZE:"
"$size" "$elf"
