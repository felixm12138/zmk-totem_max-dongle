#!/usr/bin/env sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
prospector_dir="${repo_root}/../prospector-zmk-module"

if [ ! -d "${prospector_dir}" ]; then
    prospector_dir="${repo_root}/prospector-zmk-module"
fi

if [ ! -d "${prospector_dir}" ]; then
    echo "prospector-zmk-module was not found. Run west update first." >&2
    exit 1
fi

classic_layout_dir="${prospector_dir}/boards/shields/prospector_adapter/src/layouts/classic"
if [ ! -d "${classic_layout_dir}" ]; then
    echo "Prospector classic layout directory was not found: ${classic_layout_dir}" >&2
    exit 1
fi

target="${classic_layout_dir}/battery_bar.c"
cp "${repo_root}/patches/battery_bar.c" "${target}"
grep -q "side_label" "${target}"
echo "Applied Prospector battery bar patch: ${target}"
