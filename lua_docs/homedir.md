homedir                         {#homedir}
========

The nexuslua function [homedir](homedir.md) returns the path to the user's home folder (`$HOME` under Linux and Mac OS X, and `%%USERPROFILE%` under Windows), appended with the platform-specific directory separator character.

# Example under Windows

    print(homedir()) -- C:\Users\zippr\
