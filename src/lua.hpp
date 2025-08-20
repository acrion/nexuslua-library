/*
Copyright (c) 2025 acrion innovations GmbH
Authors: Stefan Zipproth, s.zipproth@acrion.ch

This file is part of nexuslua, see https://github.com/acrion/nexuslua and https://nexuslua.org

nexuslua is offered under a commercial and under the AGPL license.
For commercial licensing, contact us at https://acrion.ch/sales. For AGPL licensing, see below.

AGPL licensing:

nexuslua is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

nexuslua is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with nexuslua. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "lua_table.hpp"

#include <filesystem>
#include <memory>
#include <string>

struct lua_State;

namespace nexuslua
{
    struct Message;
    class Agent;

    class Lua
    {
        struct Impl;
        std::unique_ptr<Impl> _impl;

    public:
        Lua(Agent* const agent);
        ~Lua();
        void                  Run(const std::filesystem::path& luaFilePath);
        void                  Run(const std::string& luaCode, const std::filesystem::path& luaFileContainingTheCode);
        std::filesystem::path GetPath() const;
        lua_State*            GetState() const;
        std::string           GetLicensee() const;
        LuaTable              RunPlugin(const Message& incomingMessage) const;

        static std::string GetVersion();
    };

    void     lua_pushvalue(lua_State* L, const cbeam::container::xpod::type& value); ///< push a \ref value onto the Lua stack; works analogous to [existing lua_push<xy> functions](https://www.lua.org/manual/5.4/manual.html#lua_pushboolean)
    void     lua_pushtable(lua_State* L, const LuaTable& parameters);                ///< push a \ref table onto the Lua stack; works analogous to [existing lua_push<xy> functions](https://www.lua.org/manual/5.4/manual.html#lua_pushboolean)
    LuaTable lua_totable(lua_State* L, int idx);                                     ///< get a \ref table from the Lua stack; works analogous to [existing lua_to<xy> functions](https://www.lua.org/manual/5.4/manual.html#lua_toboolean)
}