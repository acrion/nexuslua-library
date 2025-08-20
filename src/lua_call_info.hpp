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

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace nexuslua
{
    struct LuaCallInfo
    {
        enum class ReturnType
        {
            INVALID,
            VOID_, // workaround for VOID definition by windows.h, which we should not #undef (we undefine min and max via NOMINMAX)
            TABLE,
            LONG_LONG, // needs to match lua_Integer, see lua.h and LuaCodeGenerator.lua
            STRING,
            DOUBLE,
            VOID_PTR,
            BOOL
        };

        LuaCallInfo() = default;
        LuaCallInfo(const std::filesystem::path& dllPath, const std::string& functionName, const std::string& signature);
        virtual ~LuaCallInfo();

        std::filesystem::path                       dllPath;
        std::string                                 functionName;
        std::string                                 signature;
        std::shared_ptr<boost::dll::shared_library> dll;
        ReturnType                                  returnType{ReturnType::INVALID};
    };
}
