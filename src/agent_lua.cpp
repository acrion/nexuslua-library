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

#include "agent_lua.hpp"

#include "agent.hpp"
#include "agents.hpp"
#include "lua.hpp"

#include <cbeam/serialization/xpod.hpp>

#include <utility>

namespace nexuslua
{
    AgentLua::AgentLua(const std::shared_ptr<agents> agent_group, std::string name)
        : Agent{agent_group}
        , _name{std::move(name)}
    {
    }

    void AgentLua::Start(const std::filesystem::path& luaPath, const std::string& luaCode)
    {
        _luaPath = luaPath;
        Agent::Start(luaPath, luaCode);
    }

    std::filesystem::path AgentLua::GetLuaPath() const
    {
        return _luaPath;
    }

    std::string AgentLua::GetName() const
    {
        return _name;
    }
}
