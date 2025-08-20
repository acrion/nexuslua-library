zip {#zip}
========

The nexuslua function [zip](zip.md) creates a zip archive from the contents of a given folder. It accepts 2 parameters:

- the source directory
- the path to the output zip file

It returns a string. If the function was successful, the returned string is empty. Otherwise, it contains an error
description.

# Example

    local result = zip("testfolder", "myarchive.zip")

    if result ~= "" then
        print("Error: " .. result)
    else
        print("Success")
    end

Sample output in case testfolder does not exist:

    Error: filesystem error: directory iterator cannot open directory: No such file or directory [testfolder]
