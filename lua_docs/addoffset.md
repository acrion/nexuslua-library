addoffset {#addoffset}
========

The nexuslua function [addoffset](addoffset.md) adds an offset to a Lua variable of
type [light user data](https://www.lua.org/manual/5.4/manual.html#2.1). It accepts 3 parameters:

- the base address of type [light user data](https://www.lua.org/manual/5.4/manual.html#2.1) (in nexuslua, typically
  the result of a call to [touserdata](touserdata.md))
- an integer containing the number of elements to add
- an integer containing the byte size of an element

Therefore, the resulting memory address will be the product of the latter two values.
For an example see [poke](poke.md).

# See also

- [peek](peek.md)
- [poke](poke.md)
- [touserdata](touserdata.md)
