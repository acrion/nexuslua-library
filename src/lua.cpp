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

#include <cbeam/container/find.hpp>
#include <cbeam/convert/xpod.hpp>

#include <cbeam/convert/nested_map.hpp>

#include "lua.hpp"

#include "agent.hpp"
#include "configuration.hpp"
#include "lua_extension.hpp"
#include "message.hpp"
#include "platform_specific.hpp"
#include "utility.hpp"

#include <cbeam/container/stable_reference_buffer.hpp>
#include <cbeam/convert/string.hpp>
#include <cbeam/logging/log_manager.hpp>
#include <cbeam/platform/runtime.hpp>

extern "C"
{
#include "lauxlib.h"
#include "lualib.h"

#include "lobject.h"
#include "lstate.h"
#include "lundump.h"
}

#include <functional>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

using namespace nexuslua;
using namespace std::string_literals;

static void lstop(lua_State* L, lua_Debug* /*ar*/)
{
    lua_sethook(L, nullptr, 0, 0); // reset hook
    luaL_error(L, "interrupted!");
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"
static int msghandler(lua_State* L)
#pragma clang diagnostic pop
{
    const char* msg = lua_tostring(L, 1);
    if (msg == nullptr) // is error object not a string?
    {
        if (luaL_callmeta(L, 1, "__tostring") && // does it have a metamethod
            lua_type(L, -1) == LUA_TSTRING)      // that produces a string?
            return 1;                            // that is the message
        else
            msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1); // append a standard traceback
    return 1;                     // return the traceback
}

namespace nexuslua
{
    struct Lua::Impl
    {
        Impl(Agent* const agent)
            : _agent{agent}
            , _luaState{luaL_newstate()}
        {
            if (_luaState == nullptr)
            {
                throw std::runtime_error("cannot create Lua state");
            }

            luaL_openlibs(_luaState);

            RegisterLuaFunction("userdatadir", LuaExtension::Userdatadir);
            RegisterLuaFunction("addagent", LuaExtension::AddAgent);
            RegisterLuaFunction("addmessage", LuaExtension::AddMessage);
            RegisterLuaFunction("addoffset", LuaExtension::AddOffset);
            RegisterLuaFunction("cores", LuaExtension::Cores);
            RegisterLuaFunction("currentdir", LuaExtension::CurrentDir);
            RegisterLuaFunction("env", LuaExtension::Env);
            RegisterLuaFunction("getconfig", LuaExtension::GetConfig);
            RegisterLuaFunction("homedir", LuaExtension::HomeDir);
            RegisterLuaFunction("import", LuaExtension::Import);
            RegisterLuaFunction("install", LuaExtension::Install);
            RegisterLuaFunction("log", LuaExtension::Log);
            RegisterLuaFunction("luastate", LuaExtension::LuaState);
            RegisterLuaFunction("mktemp", LuaExtension::MkTemp);
            RegisterLuaFunction("poke", LuaExtension::Poke);
            RegisterLuaFunction("peek", LuaExtension::Peek);
            RegisterLuaFunction("printtable", LuaExtension::PrintTable);
            RegisterLuaFunction("readfile", LuaExtension::ReadFile);
            RegisterLuaFunction("isreplicated", LuaExtension::IsReplicated);
            RegisterLuaFunction("scriptdir", LuaExtension::ScriptDir);
            RegisterLuaFunction("send", LuaExtension::Send);
            RegisterLuaFunction("setconfig", LuaExtension::SetConfig);
            RegisterLuaFunction("touserdata", LuaExtension::ToUserData);
            RegisterLuaFunction("time", LuaExtension::Time);
            RegisterLuaFunction("zip", LuaExtension::Zip);
            RegisterLuaFunction("unzip", LuaExtension::Unzip);

            LuaExtension::ProvideNativeLuaFunctions(_luaState);
        }

        ~Impl()
        {
            lua_close(_luaState);
        }

        void RegisterLuaFunction(const std::string& name, lua_CFunction function)
        {
            lua_pushcfunction(_luaState, function);
            lua_setglobal(_luaState, name.c_str());
        }

        int RunLoadedLuaCode()
        {
            LuaExtension::PushRegisteredTables(_luaState);

            if (!_luaFilePath.empty())
            {
                const std::filesystem::path directory = _luaFilePath.parent_path();
                static const int            symbol_inside_runtime_binary{};
                const std::string           dll_ext = cbeam::platform::get_path_to_runtime_binary(&symbol_inside_runtime_binary).extension().string().substr(1); // "so", "dylib" or "dll"

                for (std::filesystem::path currentPath : std::filesystem::directory_iterator(directory))
                {
                    if (currentPath.extension().string() == "." + dll_ext)
                    {
                        std::string dll_name = currentPath.stem().string();
#if defined(__linux__) || defined(__APPLE__)
                        if (dll_name.find("lib") == 0)
                        {
                            dll_name = dll_name.substr(3);
                        }
#endif

                        LuaExtension::StoreDirectoryOfDll(dll_name, directory);
                    }
                }
            }

            int nArgs    = 0;
            int nResults = 0;
            int base     = lua_gettop(_luaState) - nArgs; /* function index */
            lua_pushcfunction(_luaState, msghandler);     /* push message handler */
            lua_insert(_luaState, base);                  /* put it under function and args */
            {
                std::lock_guard<std::mutex> lock(_luaStaticStateMutex);
                _luaStaticState = _luaState;
                signal(SIGINT, laction); /* set C-signal handler */
            }
            CBEAM_LOG_DEBUG("Running Lua script");
            int status = lua_pcall(_luaState, nArgs, nResults, base);
            CBEAM_LOG_DEBUG("Finished Lua script");
            signal(SIGINT, SIG_DFL);     /* reset C-signal handler */
            lua_remove(_luaState, base); /* remove message handler from the stack */

            return status;
        }

        static void laction(int i)
        {
            signal(i, SIG_DFL); /* if another SIGINT happens, terminate process */
            lua_sethook(_luaStaticState, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
        }

        Agent* const          _agent;
        std::filesystem::path _luaFilePath;
        lua_State*            _luaState{nullptr};
        mutable std::mutex    _luaStateMutex;
        static lua_State*     _luaStaticState;
        static std::mutex     _luaStaticStateMutex;
    };

    lua_State* Lua::Impl::_luaStaticState;
    std::mutex Lua::Impl::_luaStaticStateMutex;

    Lua::Lua(Agent* const agent)
        : _impl{std::make_unique<Impl>(agent)}
    {
    }

    Lua::~Lua()
    {
    }

    void Lua::Run(const std::string& luaCode, const std::filesystem::path& luaFileContainingTheCode)
    {
        _impl->_luaFilePath = luaFileContainingTheCode;

        CBEAM_LOG_DEBUG("Lua::Run(): Loading lua code from string contained in '" + _impl->_luaFilePath.string() + "'...");

        int status = luaL_loadstring(_impl->_luaState, luaCode.c_str());

        if (status == LUA_OK)
        {
            status = _impl->RunLoadedLuaCode();
        }

        if (status != LUA_OK)
        {
            std::string msg = "Lua::Run(): error loading Lua code from string: "s + lua_tostring(_impl->_luaState, -1);
            lua_pop(_impl->_luaState, 1); // remove error message
            throw std::runtime_error(msg);
        }

        CBEAM_LOG_DEBUG("Lua::Run(): Successfully ran lua code from string contained in '" + _impl->_luaFilePath.string() + "'...");
    }

    void Lua::Run(const std::filesystem::path& luaFilePath)
    {
        _impl->_luaFilePath = luaFilePath;

        if (!std::filesystem::exists(luaFilePath))
        {
            throw std::runtime_error("Lua::Run(): Missing file '" + luaFilePath.string() + "'");
        }

        CBEAM_LOG_DEBUG("Lua::Run(): Loading lua file '" + _impl->_luaFilePath.string() + "'...");

        int status = luaL_loadfile(_impl->_luaState, luaFilePath.string().c_str());

        if (status == LUA_OK)
        {
            status = _impl->RunLoadedLuaCode();
        }

        if (status != LUA_OK)
        {
            std::string msg = "Lua::Run(): error loading file "s + lua_tostring(_impl->_luaState, -1);
            lua_pop(_impl->_luaState, 1); /* remove message */
            throw std::runtime_error(msg);
        }

        CBEAM_LOG_DEBUG("Successfully ran '" + _impl->_luaFilePath.string() + "'.");
    }

    lua_State* Lua::GetState() const
    {
        return _impl->_luaState;
    }

    std::string Lua::GetLicensee() const
    {
        LuaExtension::ResetImportedFunctions();

        std::string functionName("IsLicensed");
        lua_getglobal(_impl->_luaState, functionName.c_str());
        std::string result;
        if (lua_pcall(_impl->_luaState, 0 /*arguments*/, 2 /*results*/, 0) != 0)
        {
            result = "plugin does not support licensing";
        }
        else
        {
            result = lua_tostring(_impl->_luaState, -1); // second return value
        }
        LuaExtension::ResetImportedFunctions();
        return result;
    }

    LuaTable Lua::RunPlugin(const Message& incomingMessage) const
    {
        LuaTable    result;
        const char* functionName = incomingMessage.name.c_str();

        try
        {
            const LuaTable& parameters = incomingMessage.parameters;

#if CBEAM_DEBUG_LOGGING
            {
                std::stringstream log;
                log << "Calling " << functionName << " with "
                    << parameters.data.size() << " parameters and "
                    << parameters.sub_tables.size() << " parameter tables.";

                if (_impl->_agent->GetConfiguration().GetInternal<bool>(Configuration::logMessages))
                {
                    // only log the message content if the generic logging is active, too
                    log << std::endl
                        << cbeam::convert::to_string(parameters);
                }

                CBEAM_LOG_DEBUG(log.str());
            }
#endif

            std::lock_guard<std::mutex> lock(_impl->_luaStateMutex);
            LuaExtension::ResetImportedFunctions();

            lua_getglobal(_impl->_luaState, functionName); // push function to be called
            lua_pushtable(_impl->_luaState, parameters);   // push arguments

            {
                // Reference counting of class cbeam::stable_reference_buffer does not count references held by pure Lua because a pointer in this context
                // is represented by a string (see cbeam::convert::to_string). To ensure that memory allocated via shared libraries loaded with
                // nexuslua’s `import` (LuaExtension::Import) isn't prematurely deallocated before the end of this block, we make an instance of
                // stable_reference_buffer::delay_deallocation. At that end of this block, the memory is held by managed cbeam::memory::pointer instances inside
                // the table `result`. Shared libraries that were loaded via nexuslua’s `import` are automatically unloaded (LuaCallInfo::~LuaCallInfo),
                // so memory that needs to persist (because it’s accessed by other nexuslua plugins) must be allocated by cbeam::stable_reference_buffer.
                cbeam::container::stable_reference_buffer::delay_deallocation delayDeallocation;

                if (lua_pcall(_impl->_luaState, 1 /*arguments*/, 1 /*results*/, 0) != 0)
                {
                    std::string errorMessage("Error running function '"s + functionName + "': " + lua_tostring(_impl->_luaState, -1));
                    CBEAM_LOG(errorMessage);
                    throw std::runtime_error(errorMessage);
                }

                if (lua_istable(_impl->_luaState, -1))
                {
                    result = lua_totable(_impl->_luaState, -1);
                    lua_pop(_impl->_luaState, 1); // pop returned value
                }
            }

            LuaExtension::ResetImportedFunctions();

            auto error = result.data.find("error");

            if (error != result.data.end() && std::get_if<std::string>(&error->second))
            {
                CBEAM_LOG(GetPath().string() + ": Error running function '" + functionName + "': " + std::get<std::string>(error->second) + "\n" + cbeam::convert::to_string(incomingMessage.parameters));
            }
            else
            {
                auto message = result.data.find("message");

                if (message != result.data.end())
                {
                    CBEAM_LOG_DEBUG(GetPath().string() + " -> " + functionName + ": success (" + cbeam::container::get_value_or_default<std::string>(message->second) + ")");
                }
                else
                {
                    CBEAM_LOG_DEBUG(GetPath().string() + " -> " + functionName + ": success");
                }
            }
        }
        catch (const std::exception& ex)
        {
            std::stringstream errorText;
            errorText << functionName << ": " << ex.what() << std::endl
                      << cbeam::convert::to_string(incomingMessage.parameters);
            CBEAM_LOG(errorText.str());
            result.data["error"] = errorText.str();
        }
        catch (...)
        {
            std::stringstream errorText;
            errorText << functionName << ": fatal internal error" << std::endl
                      << cbeam::convert::to_string(incomingMessage.parameters);
            CBEAM_LOG(errorText.str());
            result.data["error"] = errorText.str();
        }

        return result;
    }

    LuaTable lua_totable(lua_State* L, int idx) // NOLINT(misc-no-recursion)
    {
        nexuslua::LuaTable t;

        lua_pushnil(L);
        if (idx < 0)
        {
            --idx;
        }
        while (lua_next(L, idx) != 0)
        {
            std::string key;

            if (lua_isinteger(L, -2))
            {
                key = cbeam::convert::to_string(lua_tointeger(L, -2));
            }
            else if (lua_isnumber(L, -2))
            {
                key = cbeam::convert::to_string(lua_tonumber(L, -2));
            }
            else if (lua_isboolean(L, -2))
            {
                key = lua_toboolean(L, -2) ? "1" : "0";
            }
            else if (lua_isstring(L, -2)) // is actually 'lua_isstringornumber', see https://www.lua.org/manual/5.4/manual.html#lua_isstring
            {
                key = lua_tostring(L, -2); // may be a representation of cbeam::memory::pointer, no need to check this here
            }
            else
            {
                throw std::runtime_error("keys must be strings, integers, numbers or booleans");
            }

            if (lua_istable(L, -1))
            {
                // recursively scan all sub tables
                t.sub_tables[key] = lua_totable(L, -1);
            }
            else
            {
                if (lua_isinteger(L, -1))
                {
                    t.data[key] = lua_tointeger(L, -1);
                }
                else if (lua_isstring(L, -1)) // is actually 'lua_isstringornumber', see https://www.lua.org/manual/5.4/manual.html#lua_isstring
                {
                    if (lua_isnumber(L, -1)) // recognizes strings containing hex numbers like "0x12345" as numbers. We create such strings in cbeam::to_string(const void* val)
                    {
                        const char* value                  = lua_tostring(L, -1);
                        void*       managed_memory_address = nullptr;

                        if (value[0] == '0' && value[1] == 'x') // performance optimization to recognize numbers (double values) fast
                        {
                            managed_memory_address = cbeam::convert::from_string<void*>(value);

                            if (!cbeam::container::stable_reference_buffer::is_known(managed_memory_address))
                            {
                                managed_memory_address = nullptr;
                            }
                        }

                        if (managed_memory_address)
                        {
                            t.data[key] = cbeam::memory::pointer(managed_memory_address);
                        }
                        else
                        {
                            t.data[key] = lua_tonumber(L, -1); // double value (converted from either decimal or hex 0x... syntax)
                        }
                    }
                    else
                    {
                        t.data[key] = lua_tostring(L, -1);
                    }
                }
                else if (lua_isboolean(L, -1))
                {
                    t.data[key] = static_cast<bool>(lua_toboolean(L, -1));
                }
                else
                {
                    throw std::runtime_error("values must be tables, strings (potentially pointers), integers, numbers or booleans");
                }
            }
            lua_pop(L, 1);
        }

        return t;
    }

    void lua_pushtable(lua_State* L, const LuaTable& parameters) // NOLINT(misc-no-recursion)
    {
        lua_newtable(L);
        for (const auto& keyValue : parameters.data)
        {
            lua_pushvalue(L, keyValue.first);
            lua_pushvalue(L, keyValue.second);
            lua_settable(L, -3);
        }

        for (auto& keyValue : parameters.sub_tables)
        {
            lua_pushvalue(L, keyValue.first);
            lua_pushtable(L, keyValue.second);
            lua_settable(L, -3);
        }
    }

    void lua_pushvalue(lua_State* L, const cbeam::container::xpod::type& value)
    {
        switch (value.index())
        {
        case cbeam::container::xpod::type_index::integer:
            lua_pushinteger(L, std::get<cbeam::container::xpod::type_index::integer>(value));
            break;
        case cbeam::container::xpod::type_index::number:
            lua_pushnumber(L, std::get<cbeam::container::xpod::type_index::number>(value));
            break;
        case cbeam::container::xpod::type_index::boolean:
            lua_pushboolean(L, std::get<cbeam::container::xpod::type_index::boolean>(value));
            break;
        case cbeam::container::xpod::type_index::pointer:
            lua_pushstring(L, static_cast<std::string>(std::get<cbeam::container::xpod::type_index::pointer>(value)).c_str());
            break;
        case cbeam::container::xpod::type_index::string:
            lua_pushstring(L, std::get<cbeam::container::xpod::type_index::string>(value).c_str());
            break;
        default:
            CBEAM_LOG("Internal error: value with undefined index " + std::to_string(value.index()));
            assert(false);
            break;
        }
    }

    std::filesystem::path Lua::GetPath() const
    {
        return _impl->_luaFilePath;
    }

    std::string Lua::GetVersion()
    {
        return std::string(LUA_VERSION_MAJOR) + "." + LUA_VERSION_MINOR + "." + LUA_VERSION_RELEASE;
    }
}
