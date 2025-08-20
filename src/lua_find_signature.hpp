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

#include <boost/dll.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>

struct lua_State;

namespace nexuslua
{
    typedef std::function<void(lua_State* L, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName)> CallDllFunctionType;
    extern std::map<std::string, CallDllFunctionType>                                                                    callDllFunction; // initialized in ${CMAKE_CURRENT_BINARY_DIR}/LuaFindSignature_Map.cpp

    // generated in ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_map.cpp
    void InitCallDllFunction();

    // generated in ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_if_chain.cpp
    void CallDllFunction_void(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName);
    void CallDllFunction_table(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName);
    void CallDllFunction_long_long(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName);
    void CallDllFunction_const_charPtr(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName);
    void CallDllFunction_double(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName);
    void CallDllFunction_voidPtr(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName);
    void CallDllFunction_bool(lua_State* L, std::string signature, std::shared_ptr<boost::dll::shared_library> dll, std::string functionName);
}
