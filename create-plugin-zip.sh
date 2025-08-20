#!/usr/bin/env bash

# This script is designed to package typical nexuslua plugin files into a ZIP file. Initially, it collects the required
# files from a specified input directory using the "collect-plugin-files.sh" script. The resulting temporary directory
# is then packaged into a ZIP file. This process is particularly useful in automated build processes to create an nexuslua
# plugin installation package.

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <input_plugin_directory> <output_plugin_zip_path> <nexuslua_executable_path>"
    exit 1
fi

convert_path() {
    if ! command -v uname >/dev/null 2>&1; then
        echo "Error: 'uname' is not available but is required to determine the operating environment." >&2
        exit 1
    fi

    local os_type=$(uname -o)
    local path="$1"

    if [[ "$os_type" == "Msys" || "$os_type" == "Cygwin" ]]; then
        if command -v cygpath >/dev/null 2>&1; then
            # 'cygpath' is used because native Windows tools (like nexuslua) don't understand UNIX-style paths (e.g., /tmp/).
            # The '--mixed' option generates a more universal Windows path using forward slashes, avoiding backslash parsing issues.
            # Note: Some native Windows commands may misinterpret forward slashes, but they're preferable in this context.
            path=$(cygpath -m "$path")
        else
            echo "Error: 'cygpath' is not available but is required when running under $os_type environment." >&2
            exit 1
        fi
    fi

    echo "$path"
}

SCRIPT_DIR=$(dirname "$0")
INPUT_DIR="$1"
PLUGIN_ZIP=$(convert_path "$(realpath "$2")")
NEXUSLUA="$(which "$3")"

if [ ! -f "$NEXUSLUA" ]; then
    echo "Error: $NEXUSLUA does not exist."
    exit 1
fi

if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: $INPUT_DIR does not exist or is not a directory."
    exit 1
fi

TEMP_PLUGIN_DIR=$(convert_path "$("$SCRIPT_DIR/collect-plugin-files.sh" "$INPUT_DIR")")

# Check if the collect-plugin-files.sh script executed successfully and if the temporary directory exists.
if [ $? -ne 0 ] || [ ! -d "$TEMP_PLUGIN_DIR" ]; then
    echo "Error: Failed to collect the plugin files into the temporary directory $TEMP_PLUGIN_DIR"
    exit 1
fi

output=$("$NEXUSLUA" -e "print(zip(\"$TEMP_PLUGIN_DIR\", \"$PLUGIN_ZIP\"))")
exit_code=$?

if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to create the plugin ZIP from $TEMP_PLUGIN_DIR with exit code $exit_code"
    exit $exit_code
fi

if [ -n "$output" ]; then
    echo "$output"
    exit 1
fi

rm -rf "$TEMP_PLUGIN_DIR"
