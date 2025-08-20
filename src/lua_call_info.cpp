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

#include "lua_call_info.hpp"
#include "utility.hpp"

#include <cbeam/logging/log_manager.hpp>

using namespace nexuslua;

LuaCallInfo::LuaCallInfo(const std::filesystem::path& dllPath, const std::string& functionName, const std::string& signature)
    : dllPath(dllPath)
    , functionName(functionName)
    , signature(signature)
    , dll(std::make_shared<boost::dll::shared_library>(dllPath.string(), boost::dll::load_mode::append_decorations | boost::dll::load_mode::load_with_altered_search_path))
{
    CBEAM_LOG_DEBUG("LuaCallInfo(" + functionName + "): Incrementing reference counter of " + dllPath.string());
}

LuaCallInfo::~LuaCallInfo()
{
    if (dll.use_count() == 1)
    {
        CBEAM_LOG_DEBUG("LuaCallInfo(" + functionName + "): Decrementing reference counter of " + dllPath.string());
    }
}
