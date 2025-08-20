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

#include "lua_extension.hpp"

#include "agent.hpp"
#include "agent_lua.hpp"
#include "agents.hpp"
#include "configuration.hpp"
#include "lua.hpp"
#include "lua_call_info.hpp" // must be included prior Lua headers because they break boost header compilation
#include "utility.hpp"

#include <cbeam/convert/xpod.hpp>

#include <cbeam/container/find.hpp>
#include <cbeam/convert/nested_map.hpp>
#include <cbeam/filesystem/io.hpp>
#include <cbeam/logging/log_manager.hpp>

#include <filesystem>
#include <memory>

#include <cbeam/container/thread_safe_map.hpp>
#include <cbeam/platform/compiler_compatibility.hpp>
#include <cbeam/platform/system_folders.hpp>
#include <cbeam/random/generators.hpp>

#include "lua_table.hpp"

CBEAM_SUPPRESS_WARNINGS_PUSH()
#include "boost/algorithm/string/replace.hpp"
CBEAM_SUPPRESS_WARNINGS_POP()

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

extern "C"
{
#include "lapi.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace std::literals;

namespace nexuslua
{
    std::map<std::string, CallDllFunctionType> callDllFunction;
}

namespace nexuslua::LuaExtension
{
    std::string nativeLuaFunctionsChunk;

    std::string nativeLuaFunctions =
        R"(
        local function registerfunctions()
            function mergetables(t1, t2)
                local result = {}

                for k, v in pairs(t1) do
                    if type(v) == "table" then
                        result[k] = mergetables(v, {}) -- clone instead of copying the reference to v
                    else
                        result[k] = v
                    end
                end

                for k, v in pairs(t2) do
                    if type(v) == "table" then
                        if result[k] == nil then
                            result[k] = mergetables(v, {}) -- clone instead of copying the reference to v
                        elseif type(result[k]) == "table" then
                            result[k] = mergetables(v, result[k])
                        else
                            error("Cannot merge table with non-table value at key: " .. tostring(k))
                        end
                    elseif result[k] == nil then
                        result[k] = v
                    elseif result[k] ~= v then
                        error("Cannot merge two different non-table values at key: " .. tostring(k))
                    end
                end

                local mt1 = getmetatable(t1)
                local mt2 = getmetatable(t2)

                if mt1 then
                    setmetatable(result, mergetables(mt1, {}))
                end

                if mt2 then
                    setmetatable(result, mergetables(mt2, getmetatable(result) or {}))
                end
                return result
            end
        end

        return string.dump(registerfunctions)
        )";

    std::string GenerateBinaryChunk(std::string luaCode);

    struct Initializer
    {
        Initializer()
        {
            InitCallDllFunction();

            nativeLuaFunctionsChunk = GenerateBinaryChunk(nativeLuaFunctions); // prepare to save time when actual lua states are created
        }
    } initializer; // to fill callDllFunction as soon as this shared lib is loaded

    std::map<std::string, std::set<std::filesystem::path>> _directories_of_DLL;
    std::mutex                                             _directories_of_DLL_mutex;
    struct DataOfLuaState
    {
        Agent*      agent{nullptr};
        std::string luaPath;
        bool        isReplicated{false};
    };
    cbeam::container::thread_safe_map<lua_State*, DataOfLuaState>       _data_of_luaState;
    cbeam::container::thread_safe_map<const Agent*, nexuslua::LuaTable> _table_of_agent;

    // There may be equally named DLL functions in different Lua instances. We differ
    // between Lua instances via their thread id, so use a different map per key.
    std::map<std::thread::id, std::map<std::string, LuaCallInfo>> _importedFunction;
    std::mutex                                                    _importedFunction_mutex;

    void ResetImportedFunctions()
    {
        std::lock_guard<std::mutex> lock(_importedFunction_mutex);
        const auto&                 it = _importedFunction.find(std::this_thread::get_id());
        if (it != _importedFunction.end())
        {
            it->second.clear();
        }
    }

    void StoreImportedFunction(const LuaCallInfo& s)
    {
        // All imported functions call the same function 'CallDllFunction'.
        // We store the correct function name in map importedFunction.
        // Later in CallDllFunction, when it was called via Lua, we look up the function name in importedFunction so we know meta info like DLL name.
        std::lock_guard<std::mutex> lock(_importedFunction_mutex);
        _importedFunction[std::this_thread::get_id()][s.functionName] = s;
    }

    bool FunctionHasBeenImported(LuaCallInfo& s)
    {
        std::lock_guard<std::mutex> lock(_importedFunction_mutex);
        const auto&                 it = _importedFunction.find(std::this_thread::get_id());
        if (it != _importedFunction.end())
        {
            const auto& it2 = it->second.find(s.functionName);

            if (it2 != it->second.end())
            {
                return true;
            }
        }

        return false;
    }

    LuaCallInfo GetImportedFunction(const std::string& functionName)
    {
        std::lock_guard<std::mutex> lock(_importedFunction_mutex);
        const auto&                 it = _importedFunction.find(std::this_thread::get_id());
        if (it != _importedFunction.end())
        {
            const auto& it2 = it->second.find(functionName);

            if (it2 != it->second.end())
            {
                return it2->second;
            }
        }

        // this should never occur even in buggy scripts, because CallDllFunction is only called if the function has been registered via Import
        throw std::runtime_error("CallDllFunction: function '" + functionName + "' was called without prior call to Import(<pathToSharedLib>, " + functionName + ", <returnType>(<parameterList>)");
    }

    void StoreDirectoryOfDll(const std::string& dll_name, const std::filesystem::path& directory)
    {
        CBEAM_LOG_DEBUG("Stored path to shared library " + (directory / dll_name).string());
        std::lock_guard<std::mutex> lock(_directories_of_DLL_mutex);
        _directories_of_DLL[dll_name].insert(directory);
    }

    void StoreAgentOfLuaState(lua_State* L, Agent* agent, const std::string& luaPath, const bool isReplicated)
    {
        _data_of_luaState[L] = {agent, luaPath, isReplicated};
    }

    void RemoveAgentOfLuaState(lua_State* L)
    {
        _data_of_luaState.erase(L);
    }

    void RegisterTableForAgent(const Agent* agent, const nexuslua::LuaTable& table)
    {
        auto lock_guard = _table_of_agent.get_lock_guard();

        for (const auto& subTable : table.sub_tables)
        {
            auto it = _table_of_agent[agent].sub_tables.find(subTable.first);

            if (it != _table_of_agent[agent].sub_tables.end())
            {
                throw std::runtime_error("attempt to push duplicate table '" + cbeam::convert::to_string(subTable.first) + "' to agent '" + agent->GetName()
                                         + "'. Current table entries: '" + cbeam::convert::to_string(it->second)
                                         + "'. New table entries: '" + cbeam::convert::to_string(subTable.second) + "'.");
            }

            _table_of_agent[agent].sub_tables[subTable.first] = subTable.second;
        }
    }

    void DeregisterTablesOfAgents()
    {
        auto lock_guard = _table_of_agent.get_lock_guard();
        _table_of_agent.clear();
    }

    void PushRegisteredTables(lua_State* L)
    {
        Agent* agent = _data_of_luaState.at(L, "internal error in LuaExtension::PushRegisteredTables: no agent is known for this lua state").agent;

        auto lock_guard = _table_of_agent.get_lock_guard();

        for (const auto& subTable : _table_of_agent[agent].sub_tables)
        {
            lua_pushtable(L, subTable.second);
            lua_setglobal(L, cbeam::convert::to_string(subTable.first).c_str());
        }
    }

    std::string Normalize(std::string str)
    {
        boost::algorithm::replace_all(str, "_", "");
        boost::algorithm::replace_all(str, " ", "");
        return str;
    }

    std::filesystem::path GetDllPath(const std::string& dllName, const std::string& functionName)
    {
        std::string modDllName = dllName;

        std::lock_guard<std::mutex> lock(_directories_of_DLL_mutex);
        auto                        it = _directories_of_DLL.find(modDllName);

        if (it == _directories_of_DLL.end())
        {
            std::string fallbackDllName = "lib" + modDllName;
            it                          = _directories_of_DLL.find(fallbackDllName);
            if (it != _directories_of_DLL.end())
            {
                modDllName = fallbackDllName;
            }
        }

        if (it != _directories_of_DLL.end())
        {
            std::filesystem::path directoryOfDll;
            switch (it->second.size())
            {
            case 0:
                throw std::runtime_error("nexuslua::GetDllPath: internal error: known dll, but no stored path");
            case 1:
                directoryOfDll = *it->second.begin();
                break;
            default:
            {
                std::string normalizedDllName = Normalize(modDllName);

                for (const std::filesystem::path& p : it->second)
                {
                    std::string lastDirComponent = p.filename().string();
                    if (Normalize(lastDirComponent) == normalizedDllName)
                    {
                        if (!directoryOfDll.empty())
                        {
                            throw std::runtime_error("nexuslua::GetDllPath: ambiguous path to DLL " + modDllName);
                        }

                        directoryOfDll = p;
                    }

                    if (directoryOfDll.empty())
                    {
                        throw std::runtime_error("nexuslua::GetDllPath: ambiguous path to DLL " + modDllName);
                    }
                }

                break;
            }
            }
            CBEAM_LOG_DEBUG("CallDllFunction: DLL path: '" + (directoryOfDll / modDllName).string() + "', function: '" + functionName + "'");
            return directoryOfDll / modDllName;
        }

        CBEAM_LOG("CallDllFunction: Unknown path to DLL " + modDllName + " that defines function '" + functionName + "', trying search paths of operating system.");
        return modDllName;
    }

    int CallDllFunction(lua_State* L)
    {
        CBEAM_LOG_DEBUG("CallDllFunction: Current lua script called a function from a DLL that was previously registered.");
        lua_Debug ar;

        if (!lua_getstack(L, 0, &ar))
        {
            throw std::runtime_error("CallDllFunction: error while getting stack info");
        }

        if (!lua_getinfo(L, "n", &ar))
        {
            throw std::runtime_error("CallDllFunction: error while getting info about name of function that is to be called");
        }
        std::string functionName = ar.name;

        LuaCallInfo s = GetImportedFunction(functionName);

        //    the following check does not work, because ar.nparams is always 0 independent from the number of params in the script
        //    if (!lua_getinfo(L, "u", &ar))
        //    {
        //      throw std::runtime_error("CallDllFunction: error while getting info about name of function that is to be called");
        //    }

        //    if (ar.nparams != s.paramType.size())
        //    {
        //      throw std::runtime_error("CallDllFunction: function '"+functionName+"' has been imported with " + std::to_string(s.paramType.size()) + " parameters, but was called with " + std::to_string(ar.nparams) + " parameters");
        //    }

        const int nReturnValues = s.returnType == LuaCallInfo::ReturnType::VOID_ ? 0 : 1;

        auto callDllFunctionEntry = callDllFunction.find(s.signature);

        if (callDllFunctionEntry == callDllFunction.end())
        {
            // map does not contain this signature, fall back to runtime search of this signature
            switch (s.returnType)
            {
            case LuaCallInfo::ReturnType::VOID_:
                CallDllFunction_void(L, s.signature, s.dll, s.functionName);
                break;
            case LuaCallInfo::ReturnType::TABLE:
                CallDllFunction_table(L, s.signature, s.dll, s.functionName);
                break;
            case LuaCallInfo::ReturnType::LONG_LONG:
                CallDllFunction_long_long(L, s.signature, s.dll, s.functionName);
                break;
            case LuaCallInfo::ReturnType::STRING:
                CallDllFunction_const_charPtr(L, s.signature, s.dll, s.functionName);
                break;
            case LuaCallInfo::ReturnType::DOUBLE:
                CallDllFunction_double(L, s.signature, s.dll, s.functionName);
                break;
            case LuaCallInfo::ReturnType::VOID_PTR:
                CallDllFunction_voidPtr(L, s.signature, s.dll, s.functionName);
                break;
            case LuaCallInfo::ReturnType::BOOL:
                CallDllFunction_bool(L, s.signature, s.dll, s.functionName);
                break;
            default:
                throw std::runtime_error("CallDllFunction: function '" + functionName + "' was called with unsupported return type '" + s.signature + "'. Please file an issue on https://github.com/acrion/nexuslua/issues to support it. Currently parameters must be in the following order: table (max 1), void* (max 2), long long (max 6), double (max 3), bool (max 3), std::string (max 1).");
            }
        }
        else
        {
            callDllFunctionEntry->second(L, s.dll, s.functionName);
        }

        CBEAM_LOG_DEBUG("CallDllFunction: Success");
        return nReturnValues;
    }

    template <typename T>
    T Peek(void* address)
    {
        return *static_cast<T*>(address);
    }

    template <typename T>
    void Poke(void* address, lua_Integer value)
    {
        if (value < static_cast<lua_Integer>(std::numeric_limits<T>::lowest()))
        {
            value += static_cast<lua_Integer>(std::numeric_limits<T>::max() - std::numeric_limits<T>::lowest());
        }
        else if (value > static_cast<lua_Integer>(std::numeric_limits<T>::max()))
        {
            value -= static_cast<lua_Integer>(std::numeric_limits<T>::max() - std::numeric_limits<T>::lowest());
        }

        *static_cast<T*>(address) = static_cast<T>(value);
    }

    template <typename T>
    void Poke(void* address, lua_Number value)
    {
        if (value < static_cast<lua_Integer>(std::numeric_limits<T>::lowest()))
        {
            value += static_cast<lua_Integer>(std::numeric_limits<T>::max() - std::numeric_limits<T>::lowest());
        }
        else if (value > static_cast<lua_Integer>(std::numeric_limits<T>::max()))
        {
            value -= static_cast<lua_Integer>(std::numeric_limits<T>::max() - std::numeric_limits<T>::lowest());
        }

        *static_cast<T*>(address) = static_cast<T>(value);
    }

    void AddMessage(Agent* agent, const std::string& luaPath, const std::string& messageName, const LuaTable& parameters)
    {
        const LuaTable::table_of_values& data      = parameters.data;
        const LuaTable::nested_tables&   subTables = parameters.sub_tables;

        auto itIconPath              = data.find("icon");
        auto itDisplayName           = data.find("displayname");
        auto itDescription           = data.find("itDescription");
        auto itParameterDescriptions = subTables.find("parameters");

        const std::string              iconPath              = itIconPath == data.end() || cbeam::container::get_value_or_default<std::string>(itIconPath->second).empty()
                                                                 ? ""
                                                                 : (std::filesystem::path(luaPath).parent_path() / cbeam::container::get_value_or_default<std::string>(itIconPath->second)).string();
        const std::string              displayName           = itDisplayName == data.end() ? "" : cbeam::container::get_value_or_default<std::string>(itDisplayName->second);
        const std::string              description           = itDescription == data.end() ? "" : cbeam::container::get_value_or_default<std::string>(itDescription->second);
        const LuaTable::nested_tables& parameterDescriptions = itParameterDescriptions == subTables.end() ? LuaTable::nested_tables() : itParameterDescriptions->second.sub_tables;

        if (!iconPath.empty() && !std::filesystem::exists(iconPath))
        {
            throw std::runtime_error("Message '" + cbeam::container::get_value_or_default<std::string>(itDisplayName->second) + "' of Lua agent '" + luaPath + "' is specifying a non-existant SVG icon " + iconPath);
        }

        agent->AddMessage(messageName, parameterDescriptions, displayName, description, iconPath);

        CBEAM_LOG_DEBUG("Added message '" + messageName + "' of " + luaPath);
    }

    int Userdatadir(lua_State* L)
    {
        lua_pushstring(L, (cbeam::filesystem::get_user_data_dir().string() + std::string(1, std::filesystem::path::preferred_separator)).c_str());
        return 1;
    }

    int AddAgent(lua_State* L)
    {
        if (!lua_isstring(L, 1) || !lua_isstring(L, 2))
        {
            throw std::runtime_error("Function addagent expects two parameters: the name of the new agent and a string containing Lua code");
        }

        auto        data      = _data_of_luaState.at(L, "internal error: lua script called 'add_agent', but no agent is known for this lua state");
        std::string luaPath   = data.luaPath;
        std::string agentName = lua_tostring(L, 1);
        std::string luaCode   = lua_tostring(L, 2);

        auto newAgent = data.agent->GetAgents()->Add(agentName, luaPath, luaCode);

        if (lua_gettop(L) >= 3 && lua_istable(L, 3))
        {
            lua_len(L, 3);
            int table_length = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);

            for (int i = 1; i <= table_length; ++i)
            {
                lua_rawgeti(L, 3, i);
                const char* messageName = lua_tostring(L, -1);
                lua_pop(L, 1);

                LuaTable parameters;

                if (messageName)
                {
                    AddMessage(newAgent.get(), luaPath, messageName, parameters);
                }
            }
        }
        else
        {
            throw std::runtime_error("Error: The optional 3rd parameter of addagent must be a table of message names.");
        }

        return 0;
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"
    int AddMessage(lua_State* L)
    {
        Agent*      agent;
        std::string luaPath;

        {
            auto lock_guard = _data_of_luaState.get_lock_guard();
            auto data       = _data_of_luaState.find(L, "internal error: lua script called 'add_message', but no agent is known for this lua state");

            if (data->second.isReplicated)
            {
                CBEAM_LOG_DEBUG("Ignoring call to add_message from script in replicated state - this message can be avoided by checking state with Lua function replicated()");
                return 0;
            }

            agent   = data->second.agent;
            luaPath = data->second.luaPath;
        }

        std::string messageName;

        if (lua_type(L, 1) != LUA_TSTRING) // e.g. LUA_TFUNCTION
        {
            throw std::runtime_error("nexuslua::AddMessage: message name has to be a string containing the function name");
        }
        else
        {
            messageName = lua_tostring(L, 1);
        }

        LuaTable parameters;

        if (lua_istable(L, 2))
        {
            parameters = lua_totable(L, 2);
        }

        AddMessage(agent, luaPath, messageName, parameters);

        return 0;
    }
#pragma clang diagnostic pop

    int AddOffset(lua_State* L)
    {
        void*       address = static_cast<void*>(lua_touserdata(L, 1));
        lua_Integer offset  = lua_tointeger(L, 2);
        lua_Integer bytes   = lua_tointeger(L, 3);
        void*       result;

        switch (bytes)
        {
        case 0:
        case 1:
            result = static_cast<uint8_t*>(address) + offset;
            break;
        case 2:
            result = static_cast<uint16_t*>(address) + offset;
            break;
        case 4:
            result = static_cast<uint32_t*>(address) + offset;
            break;
        case 8:
            result = static_cast<uint64_t*>(address) + offset;
            break;
        case -8:
            result = static_cast<double*>(address) + offset;
            break;
#ifdef __SIZEOF_INT128__
        case 16:
    #if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
    #endif
            result = static_cast<unsigned __int128*>(address) + offset;
    #if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif
            break;
        default:
            throw std::runtime_error("Error running function 'AddOffset': Number of bytes must be either 1,2,4,8,16 or -8 (for floating point). " + std::to_string(bytes) + " is not supported.");
#else
    #pragma warning(suppress : 4702) // unreachable code
            throw std::runtime_error("Error running function 'Poke': Number of bytes must be either 1,2,4,8 or -8 (for floating point). On other platforms, 16 is supported in addition. " + std::to_string(bytes) + " is not supported.");
#endif
        }

#pragma warning(suppress : 4701 4703)     // Suppressing warning for potentially uninitialized local (pointer) variable 'result'. Note: This suppression seems to be ineffective in some MSVC versions. No such warning is observed in Clang or GCC.
        lua_pushlightuserdata(L, result); // push result

        return 1; /* number of results */
    }

    int Cores(lua_State* L)
    {
        lua_pushinteger(L, std::thread::hardware_concurrency());

        return 1;
    }

    int CurrentDir(lua_State* L)
    {
        lua_pushstring(L, (std::filesystem::current_path().string() + std::string(1, std::filesystem::path::preferred_separator)).c_str());
        return 1;
    }

    int Env(lua_State* L)
    {
        if (!lua_isstring(L, 1))
        {
            throw std::runtime_error("Function env expects the name of an environment variable as argument");
        }

        const char* envVariableName = lua_tostring(L, 1);
#pragma warning(suppress : 4996) // MSVC suggests a non-portable alternative for this standard function.
        const char* homeValue = std::getenv(envVariableName);
        lua_pushstring(L, homeValue ? homeValue : "");
        return 1;
    }

    int GetConfig(lua_State* L)
    {
        auto& configuration = _data_of_luaState.at(L, "internal error in LuaExtension::GetConfig: no agent is known for this lua state").agent->GetConfiguration();
        lua_pushtable(L, configuration.GetTable());
        return 1;
    }

    int HomeDir(lua_State* L)
    {
        lua_pushstring(L, (cbeam::filesystem::get_home_dir().string() + std::string(1, std::filesystem::path::preferred_separator)).c_str());
        return 1;
    }

    int Import(lua_State* L)
    {
        CBEAM_LOG_DEBUG("Import: Current lua script is registering a function from a DLL - initializing...");

        const char* dllName      = lua_tostring(L, 1);
        const char* functionName = lua_tostring(L, 2);
        const char* signature    = lua_tostring(L, 3);

        std::filesystem::path dllPath(GetDllPath(dllName, functionName));
        LuaCallInfo           s;

        try
        {
            s = LuaCallInfo(dllPath, functionName, signature);
        }
        catch (const std::exception& ex)
        {
            CBEAM_LOG("Import: Could not load shared library '" + dllPath.string() + "': " + ex.what());
            throw ex;
        }

        s.signature            = utility::RemoveWsFromParams(s.signature);
        std::string returnType = s.signature.substr(0, s.signature.find('('));

        CBEAM_LOG_DEBUG("Import: Registering function '" + s.functionName + "' in '" + dllName + "' with signature '" + s.signature + "'");

        if (returnType == "void")
            s.returnType = LuaCallInfo::ReturnType::VOID_;
        else if (returnType == "table")
            s.returnType = LuaCallInfo::ReturnType::TABLE;
        else if (returnType == "long long") // needs to match lua_Integer, see lua.h and test/test_lua.cpp
            s.returnType = LuaCallInfo::ReturnType::LONG_LONG;
        else if (returnType == "const char*")
            s.returnType = LuaCallInfo::ReturnType::STRING;
        else if (returnType == "double")
            s.returnType = LuaCallInfo::ReturnType::DOUBLE; // needs to match lua_Number, see lua.h and test/test_lua.cpp
        else if (returnType == "void*")
            s.returnType = LuaCallInfo::ReturnType::VOID_PTR;
        else if (returnType == "bool")
            s.returnType = LuaCallInfo::ReturnType::BOOL;
        else if (returnType == "int")
            throw std::runtime_error("Import: Return type 'int' is not supported, please use 'long long' instead (matching type lua_Integer)");
        else
            throw std::runtime_error("Import: Unsupported return type '" + returnType + "'. Supported types are void, table, long long, std::string, double, void* and bool.");

        if (FunctionHasBeenImported(s))
        {
            throw std::runtime_error("Import: Function '" + s.functionName + "' is registered more than once");
        }

        StoreImportedFunction(s);
        lua_pushcfunction(L, CallDllFunction);
        lua_setglobal(L, s.functionName.c_str());

        CBEAM_LOG_DEBUG("import: Success");
        return 0; // number of results of Import (it is called from Lua)
    }

    int Install(lua_State* L)
    {
        if (!lua_isstring(L, 1))
        {
            throw std::runtime_error("Function install expects a path to a zip file containing an nexuslua plugin");
        }

        const char* pluginZipPath = lua_tostring(L, 1);
        auto        tempDir       = cbeam::filesystem::create_unique_temp_dir();
        utility::Unzip(pluginZipPath, tempDir);
        std::string errorString;
        auto        data   = _data_of_luaState.at(L, "internal error: current Lua function called `install`, but no Lua state is known for this script.");
        const auto  result = (int)data.agent->GetAgents()->InstallPlugin(tempDir, errorString);
        lua_pushinteger(L, result);
        return 1;
    }

    int Log(lua_State* L)
    {
        CBEAM_LOG(lua_tostring(L, 1));
        return 0; /* number of results */
    }

    int LuaState(lua_State* L)
    {
        lua_pushlightuserdata(L, L);
        return 1;
    }

    int MkTemp(lua_State* L)
    {
        lua_pushstring(L, cbeam::filesystem::create_unique_temp_dir().string().c_str());
        return 1;
    }

    int Peek(lua_State* L)
    {
        void*       address = static_cast<void*>(lua_touserdata(L, 1));
        lua_Integer bytes   = lua_tointeger(L, 2);

        if (bytes >= 0)
        {
            lua_Integer result;

            switch (bytes)
            {
            case 0:
            case 1:
                result = static_cast<lua_Integer>(Peek<uint8_t>(address));
                break;
            case 2:
                result = static_cast<lua_Integer>(Peek<uint16_t>(address));
                break;
            case 4:
                result = static_cast<lua_Integer>(Peek<uint32_t>(address));
                break;
            case 8:
                result = static_cast<lua_Integer>(Peek<uint64_t>(address));
                break;
            default:
                throw std::runtime_error("Error running function 'Peek': Number of bytes must be either 1,2,4,8 or -8 (for floating point). " + std::to_string(bytes) + " is not supported.");
            }

            lua_pushinteger(L, result); // push result
        }
        else
        {
            if (bytes != -8)
            {
                throw std::runtime_error("Error running function 'Peek': Number of bytes must be either 1,2,4,8 or -8 (for floating point). " + std::to_string(bytes) + " is not supported.");
            }

            lua_Number result = static_cast<lua_Number>(Peek<double>(address));
            lua_pushnumber(L, result); // push result
        }

        return 1; // number of results
    }

    int Poke(lua_State* L)
    {
        void*       address = static_cast<void*>(lua_touserdata(L, 1));
        lua_Integer bytes   = lua_tointeger(L, 3);

        if (bytes >= 0)
        {
            lua_Integer value = lua_tointeger(L, 2);

            switch (bytes)
            {
            case 0:
            case 1:
                Poke<uint8_t>(address, value);
                break;
            case 2:
                Poke<uint16_t>(address, value);
                break;
            case 4:
                Poke<uint32_t>(address, value);
                break;
            case 8:
                Poke<uint64_t>(address, value);
                break;
#ifdef __SIZEOF_INT128__
            case 16:
    #if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
    #endif
                Poke<unsigned __int128>(address, value);
    #if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif
                break;
            default:
                throw std::runtime_error("Error running function 'Poke': Number of bytes must be either 1,2,4,8,16 or -8 (for floating point). " + std::to_string(bytes) + " is not supported.");
#else
    #pragma warning(suppress : 4702) // unreachable code
                throw std::runtime_error("Error running function 'Poke': Number of bytes must be either 1,2,4,8 or -8 (for floating point). On other platforms, 16 is supported in addition. " + std::to_string(bytes) + " is not supported.");
#endif
            }
        }
        else
        {
            lua_Number value = lua_tonumber(L, 2);

            if (bytes != -8)
            {
                throw std::runtime_error("Error running function 'Poke': Number of bytes must be either 1,2,4,8,16 or -8. " + std::to_string(bytes) + " is not supported.");
            }

            Poke<double>(address, value);
        }

        return 0; /* number of results */
    }

    int ReadFile(lua_State* L)
    {
        if (!lua_isstring(L, 1))
        {
            throw std::runtime_error("Function readfile expects a string containing the path as parameter");
        }

        std::filesystem::path pathToFile = lua_tostring(L, 1);

        if (!pathToFile.is_absolute())
        {
            auto                  data                  = _data_of_luaState.at(L, "internal error: lua script called 'readfile' with a relative path, but cannot find parent directory of current Lua script file.");
            std::filesystem::path folderOfCurrentScript = std::filesystem::path(data.luaPath).parent_path();

            pathToFile = folderOfCurrentScript / pathToFile;
        }

        lua_pushstring(L, cbeam::filesystem::read_file(pathToFile).c_str());
        return 1;
    }

    int IsReplicated(lua_State* L)
    {
        auto data = _data_of_luaState.at(L, "internal error: lua script called 'replicated', but no agent is known for this lua state");
        lua_pushboolean(L, data.isReplicated);

        return 1;
    }

    int PrintTable(lua_State* L)
    {
        if (!lua_istable(L, 1))
        {
            throw std::runtime_error("Argument of function printtable must be a lua table");
        }

        std::cout << cbeam::convert::to_string(lua_totable(L, 1));
        return 0;
    }

    int ScriptDir(lua_State* L)
    {
        auto data = _data_of_luaState.at(L, "internal error: current Lua function called `scriptdir`, but no Lua state is known for this script.");

        std::shared_ptr<Agent>    agent    = data.agent->GetAgents()->GetAgent("main"s); // this agent name is set in the command line processor src/main.cpp, function process_cmd_line()
        std::shared_ptr<AgentLua> luaAgent = std::dynamic_pointer_cast<AgentLua>(agent);
        lua_pushstring(L, luaAgent ? (luaAgent->GetLuaPath().parent_path().string() + std::string(1, std::filesystem::path::preferred_separator)).c_str() : "");
        return 1;
    }

    int Send(lua_State* L)
    {
        CBEAM_LOG_DEBUG("Lua script called send");
        const char* agentName   = lua_tostring(L, 1);
        const char* messageName = lua_tostring(L, 2);
        LuaTable    parameters  = lua_totable(L, 3);

        if (parameters.GetReplyToAgentNameOrEmpty().empty())
        {
            parameters.SetReplyToAgentName(agentName);
        }

        auto data = _data_of_luaState.at(L, "internal error: current Lua function called `send`, but no Lua state is known for this script.");
        data.agent->GetAgents()->GetMessage(agentName, messageName).Send(parameters);

        return 0;
    }

    int SetConfig(lua_State* L)
    {
        if (!lua_istable(L, 1))
        {
            throw std::runtime_error("Argument of function setconfig must be a lua table");
        }

        auto data = _data_of_luaState.at(L, "internal error: current Lua function called `setconfig`, but no Lua state is known for this script.");
        data.agent->GetConfiguration().SetTable(lua_totable(L, 1));
        return 0;
    }

    int Time(lua_State* L)
    {
        using HighResClockDuration  = decltype(std::chrono::high_resolution_clock::now().time_since_epoch());
        using HighResClockTimePoint = std::chrono::time_point<std::chrono::high_resolution_clock, HighResClockDuration>;

        static const auto                  system_reference_duration = std::chrono::system_clock::from_time_t(0).time_since_epoch();
        static const HighResClockTimePoint high_res_reference_time   = HighResClockTimePoint(HighResClockDuration(system_reference_duration.count()));

        const auto duration = std::chrono::high_resolution_clock::now() - high_res_reference_time;

        const auto duration_in_10e_minus8_seconds = std::chrono::duration_cast<std::chrono::duration<lua_Integer, std::ratio<1, 100000000>>>(duration).count();

        lua_pushinteger(L, duration_in_10e_minus8_seconds);
        return 1;
    }

    int ToUserData(lua_State* L)
    {
        std::size_t       len;
        const char*       cInput = lua_tolstring(L, 1, &len);
        std::string       input(cInput, 0, len); // NOLINT(bugprone-string-constructor)
        std::stringstream stream(input);
        void*             result;
        stream >> result;

        lua_pushlightuserdata(L, result); /* push result */

        return 1; // number of results
    }

    int Unzip(lua_State* L)
    {
        if (!lua_isstring(L, 1) || !lua_isstring(L, 2))
        {
            throw std::runtime_error("Function unzip requires 2 string arguments: the path to the input zip file and the path to the output folder");
        }

        const std::string pathToZipFile = lua_tostring(L, 1);
        const std::string targetPath    = lua_tostring(L, 2);

        std::string error;

        try
        {
            utility::Unzip(pathToZipFile, targetPath);
        }
        catch (const std::exception& ex)
        {
            error = ex.what();
        }
        lua_pushstring(L, error.c_str());
        return 1;
    }

    int Zip(lua_State* L)
    {
        if (!lua_isstring(L, 1) || !lua_isstring(L, 2))
        {
            throw std::runtime_error("Function zip requires 2 string arguments: the path to the input folder and the path to the output zip file");
        }

        const std::string sourcePath    = lua_tostring(L, 1);
        const std::string pathToZipFile = lua_tostring(L, 2);

        std::string error;

        try
        {
            utility::Zip(sourcePath, pathToZipFile);
        }
        catch (const std::exception& ex)
        {
            error = ex.what();
        }
        lua_pushstring(L, error.c_str());
        return 1;
    }

    std::string GenerateBinaryChunk(std::string luaCode)
    {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);

        if (luaL_dostring(L, luaCode.c_str()) != LUA_OK)
        {
            lua_close(L);
            throw std::runtime_error(lua_tostring(L, -1));
        }

        size_t      len;
        const char* binaryChunk = lua_tolstring(L, -1, &len);
        if (binaryChunk == nullptr)
        {
            lua_close(L);
            throw std::runtime_error("Failed to generate binary chunk");
        }

        std::string binaryChunkStr(binaryChunk, len);
        lua_pop(L, 1);

        lua_close(L);
        return binaryChunkStr;
    }

    void LoadBinaryChunk(lua_State* L, const std::string& binaryChunk)
    {
        if (luaL_loadbuffer(L, binaryChunk.data(), binaryChunk.size(), "function") != LUA_OK)
        {
            throw std::runtime_error(lua_tostring(L, -1));
        }
        if (lua_pcall(L, 0, 0, 0) != LUA_OK)
        {
            throw std::runtime_error(lua_tostring(L, -1));
        }
    }

    void ProvideNativeLuaFunctions(lua_State* L)
    {
        LoadBinaryChunk(L, nativeLuaFunctionsChunk);
    }
}
