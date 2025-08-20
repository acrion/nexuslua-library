peek                         {#peek}
========

The nexuslua function [peek](peek.md) reads an integer number from a memory address. It accepts 2 parameters:

- the memory address as [light user data](https://www.lua.org/manual/5.4/manual.html#2.1)
- the number of bytes to read (1,2,4 or 8), which will be interpreted as a single integer

It returns the read integer. For an example, see [poke](poke.md).

# See also

- [addoffset](addoffset.md)
- [poke](poke.md)
- [touserdata](touserdata.md)


