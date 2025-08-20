#!/usr/bin/env bash

if [ "$#" -ne 1 ]; then
    echo "Usage: collect-plugin-files.sh <source directory>."
    echo
    echo "This script is designed to handle typical nexuslua plugin files. It copies these files from a specified source"
    echo "directory (build directory) to a temporary directory, then prints the path to this temporary directory. The primary"
    echo "use of this script is within automated build processes, where it enables the creation of an nexuslua plugin"
    echo "installation package by zipping the temporary directory. During the build process, the necessary binary files are"
    echo "already placed next to main.lua in the plugin's root directory."
    exit 1
fi

src_dir=$1

tmp_dir=$(mktemp -d)

cp "$src_dir"/main.lua "$tmp_dir" || exit 1 # required file
cp "$src_dir"/nexuslua_plugin.toml "$tmp_dir" || exit 1 # required file
cp "$src_dir"/LICENSE "$tmp_dir" 2>/dev/null
cp "$src_dir"/README.md "$tmp_dir" 2>/dev/null
cp "$src_dir"/*.so "$tmp_dir" 2>/dev/null
cp "$src_dir"/*.so.* "$tmp_dir" 2>/dev/null
cp "$src_dir"/*.dll "$tmp_dir" 2>/dev/null
cp "$src_dir"/*.dylib "$tmp_dir" 2>/dev/null
cp "$src_dir"/*.svg "$tmp_dir" 2>/dev/null

echo "$tmp_dir"
