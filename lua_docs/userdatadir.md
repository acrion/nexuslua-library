userdatadir {#userdatadir}
========

The nexuslua function [userdatadir](userdatadir.md) returns the path to the user's app data folder, i. e.

- `$HOME/.local/share/` under Linux,
- `$HOME/Library/Application Support/` under Darwin
- `%%USERPROFILE%\` under Windows.

# Example under Windows

    print(userdatadir()) -- C:\Users\zippr\AppData\Roaming\