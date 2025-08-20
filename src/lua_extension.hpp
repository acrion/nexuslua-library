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

#include "lua_find_signature.hpp"

#include <filesystem>
#include <map>
#include <mutex>

struct lua_State;

namespace nexuslua
{
    struct LuaTable;
    class Agent;

    namespace LuaExtension
    {
        int Userdatadir(lua_State* L);
        int AddAgent(lua_State* L);
        int AddMessage(lua_State* L);
        int AddOffset(lua_State* L);
        int Cores(lua_State* L);
        int CurrentDir(lua_State* L);
        int Env(lua_State* L);
        int GetConfig(lua_State* L);
        int HomeDir(lua_State* L);
        int Import(lua_State* L);
        int Install(lua_State* L);
        int Log(lua_State* L);
        int LuaState(lua_State* L);
        int MkTemp(lua_State* L);
        int Peek(lua_State* L);
        int Poke(lua_State* L);
        int ReadFile(lua_State* L);
        int IsReplicated(lua_State* L);
        int PrintTable(lua_State* L);
        int ScriptDir(lua_State* L);
        int Send(lua_State* L);
        int SetConfig(lua_State* L);
        int Time(lua_State* L);
        int ToUserData(lua_State* L);
        int Unzip(lua_State* L);
        int Zip(lua_State* L);

        void ProvideNativeLuaFunctions(lua_State* L);

        void RegisterTableForAgent(const Agent* agent, const nexuslua::LuaTable& table);
        void DeregisterTablesOfAgents();
        void PushRegisteredTables(lua_State* L);
        void ResetImportedFunctions();
        void StoreDirectoryOfDll(const std::string& dll_name, const std::filesystem::path& directory);
        void StoreAgentOfLuaState(lua_State* L, Agent* agent, const std::string& luaPath, const bool isReplicated);
    }
}
