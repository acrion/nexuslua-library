#!/usr/bin/env lua

--[[
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
--]]

if #arg<2 then
    print("Usage: lua_code_generator.lua ${CMAKE_CURRENT_BINARY_DIR}/LuaFindSignature_Map.cpp ${CMAKE_CURRENT_BINARY_DIR}/LuaFindSignature_IfChain.cpp")
    os.exit(1)
end

local max_arguments = 3
local n_use_map     = 1
local cpp_argument_types = {
    {max_sequence=max_arguments, types={"table"}}, -- see cbeam::serialization::serialized_object
    {max_sequence=max_arguments, types={"void*"}},
    {max_sequence=max_arguments, types={"long long"}}, -- needs to match lua_Integer, see lua.h
    {max_sequence=max_arguments, types={"double"}},
    {max_sequence=max_arguments, types={"bool"}},
    {max_sequence=max_arguments, types={"const char*"}}}

local concat_tables = function(table1, table2)
    local result = table.move(table1, 1, #table1, 1, {})
    table.move(table2, 1, #table2, #result+1, result)
    return result
end

local cpp_argument_type_list = concat_tables(cpp_argument_types[1].types, concat_tables(cpp_argument_types[2].types, concat_tables(cpp_argument_types[3].types, concat_tables(cpp_argument_types[4].types, concat_tables(cpp_argument_types[5].types, cpp_argument_types[6].types)))))
local cpp_return_types = table.move(cpp_argument_type_list, 1, #cpp_argument_type_list, 2, {"void"})

local mapCpp     = io.open(arg[1], "w")
local ifChainCpp = io.open(arg[2], "w")

local create_code = function(return_type, arguments)
    local strSignature = return_type.."("..table.concat(arguments, ",")..")"

    local indentation
    if #arguments <= n_use_map then
        mapCpp    :write('  callDllFunction["',strSignature,'"]')
        indentation = (n_use_map+1) * 12
    else
        ifChainCpp:write('  if (signature=="',strSignature,'")')
        indentation = (max_arguments+1) * 12
    end

    local spaces = ''
    for i=1,indentation-#strSignature do spaces=spaces.." " end

    if #arguments <= n_use_map then
        mapCpp:write(spaces,' = [](lua_State* L, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName)')
    else
        ifChainCpp:write(spaces)
    end

    local cmd = " { "

    if return_type ~= "void" then
        cmd = cmd .. "lua_push"
        if return_type=="long long"   then cmd=cmd.."integer"
        elseif return_type=="const char*" then cmd=cmd.."string"
        elseif return_type=="double"      then cmd=cmd.."number"
        elseif return_type=="void*"       then cmd=cmd.."lightuserdata"
        elseif return_type=="bool"        then cmd=cmd.."boolean"
        elseif return_type=="table"       then cmd=cmd.."table"
        end
        cmd = cmd .. "(L, "
    end

    if return_type=="table" then
        cmd=cmd.."LuaTable(" -- deserialize the type `cbeam::table` returned by the shared library
    end

    cmd = cmd .. "boost::dll::import_symbol<" .. strSignature .. ">" .. spaces .. "(*dll, functionName)("
    
    local argument_list=""
    for i,argument in ipairs(arguments) do
        if argument_list~="" then argument_list=argument_list.."," end
        if argument=="long long"   then argument_list=argument_list.."lua_tointeger(L,"..i..")"
        elseif argument=="const char*" then argument_list=argument_list.."lua_tostring(L,"..i..")"
        elseif argument=="double"      then argument_list=argument_list.."lua_tonumber(L,"..i..")"
        elseif argument=="void*"       then argument_list=argument_list.."lua_touserdata(L,"..i..")"
        elseif argument=="bool"        then argument_list=argument_list.."lua_toboolean(L,"..i..")"
        elseif argument=="table"       then argument_list=argument_list.."cbeam::serialization::serialize<cbeam::container::nested_map<cbeam::container::xpod::type, cbeam::container::xpod::type>>((cbeam::container::nested_map<cbeam::container::xpod::type, cbeam::container::xpod::type>)lua_totable(L,"..i..")).safe_get()" -- serialize the type `nexuslua::LuaTable` to pass it to the shared library function
        end
    end
        
    cmd=cmd..argument_list..")"
    
    if return_type=="table" then
        cmd=cmd..")" -- closing bracket of the extra LuaTable constructor call required to cbeam::serialization::deserialize
    end
    
    if return_type~="void" then
        cmd=cmd..")"
    end
    
    if #arguments <= n_use_map then
        mapCpp:write(cmd)
        mapCpp:write("; };\n")
    else
        ifChainCpp:write(cmd)
        ifChainCpp:write("; return; }\n")
    end
end

local sumOfArgs = 0

traverse = function(return_type, arguments_head, arguments_current, index, do_create_code)
    total_arguments = concat_tables(arguments_head, arguments_current)
    
    if do_create_code then
        create_code(return_type, total_arguments)
    end

    if #total_arguments < max_arguments then
        if index < #cpp_argument_types then
            local new_arguments_head = concat_tables(arguments_head, arguments_current)
            traverse(return_type, new_arguments_head, {}, index+1, false)
        end

        if #arguments_current < cpp_argument_types[index].max_sequence then
            for i,arg_type in ipairs(cpp_argument_types[index].types) do
                traverse(return_type,arguments_head, table.move(arguments_current, 1, #arguments_current, 2, {arg_type}), index, true) -- prepend arg_type to arguments_current and call traverse
            end
        end
    end
end

script_path = function()
   local str = debug.getinfo(2, "S").source:sub(2)
   return str:match("(.*/)")
end

local header =     '// Generated by '..debug.getinfo(1,'S').source..'\n'
header = header .. '// Time of generation: '..os.date()..'\n'
header = header .. '// Script path: '..script_path()..'\n'
-- header = header .. io.popen('git log -C "' .. script_path() .. '" -1 --pretty="// Latest git commit:  %cd %B"'):read() .. '\n'
header = header .. [[
#include "lua_find_signature.hpp"

#include <cbeam/container/xpod.hpp>

#include <cbeam/serialization/xpod.hpp>

#include <cbeam/container/nested_map.hpp>

#include <cbeam/serialization/nested_map.hpp>

#include <cbeam/serialization/direct.hpp>

#include "lua.hpp"
#include "lua_table.hpp"

#include <boost/dll.hpp> // must be included prior Lua headers because they break boost header compilation

#define luac_c
#define LUA_CORE

extern "C"
{
#include "lprefix.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lobject.h"
#include "lstate.h"
#include "lundump.h"
}

namespace nexuslua
{
    /// `table` a serialized version of an nexuslua::LuaTable instance.
    /// This type is used as a parameter or return type in function signatures of shared
    /// libraries imported via the nexuslua `import` function, facilitating communication
    /// between Lua and C++ code. The memory block to which the void* points is managed
    /// by cbeam::Memory and contains a serialized representation of a
    /// LuaTable instance, which can be deserialized back into a LuaTable object.
    using table = ::cbeam::serialization::serialized_object;
    using LuaTable = ::nexuslua::LuaTable;

]]

mapCpp:write(header)
ifChainCpp:write(header)

mapCpp:write('void InitCallDllFunction()\n')
mapCpp:write('{\n')

for i,return_type in ipairs(cpp_return_types) do
    str_return_type = return_type:gsub("*","Ptr"):gsub(' ','_'):gsub("::","_")
    ifChainCpp:write('void CallDllFunction_',str_return_type,'(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName)\n')
    ifChainCpp:write('{\n')
    traverse(return_type, {}, {}, 1, true)
    ifChainCpp:write('  throw std::runtime_error("CallDllFunction: function "+functionName+" was called with an unsupported signature "+signature+". If this signature would be useful for you, please file an issue on https://github.com/acrion/nexuslua/issues to support it. Currently parameters must be in the following order: table (max ' .. cpp_argument_types[1].max_sequence .. '), void* (max ' .. cpp_argument_types[2].max_sequence .. '), long long (max ' .. cpp_argument_types[3].max_sequence .. '), double (max ' .. cpp_argument_types[4].max_sequence .. '), bool (max ' .. cpp_argument_types[5].max_sequence .. '), const char* (max ' .. cpp_argument_types[6].max_sequence .. '). Note that type int is not supported, please use long long instead (matching type lua_Integer)");\n')
    ifChainCpp:write('}\n')
end

mapCpp:write('}\n')
local namespaceClosingBracket = ('} // namespace nexuslua\n')
mapCpp:write(namespaceClosingBracket)
ifChainCpp:write(namespaceClosingBracket)
mapCpp:close()
ifChainCpp:close()
