luastate                         {#luastate}
========

The nexuslua function [luastate](luastate.md) returns the current [Lua State](https://www.lua.org/manual/5.4/manual.html#lua_State) as [light user data](https://www.lua.org/manual/5.4/manual.html#2.1). It may be used in shared libraries that are called via [import](import.md) and use the [Lua C API](https://www.lua.org/manual/5.4/manual.html#4) themselves.