unzip {#unzip}
========

The nexuslua function [unzip](unzip.md) extracts the given zip file to a target directory. It accepts 2 parameters:

- the path to the input zip file
- the target directory

It returns a string. If the function was successful, the returned string is empty. Otherwise, it contains an error
description.

# Example

    local result = unzip("myarchive.zip", ".")

    if result ~= "" then
        print("Error: " .. result)
    else
        print("Success")
    end

Output in case myarchive.zip does not exist:

    Error: Could not open zip file: 'myarchive.zip': 9
