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

#include <cbeam/memory/pointer.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace nexuslua
{
    /// \brief stores internal and user configuration
    /// \details The configuration be accessed via nexuslua \ref setconfig and \ref getconfig or via C++.
    class Configuration
    {
    public:
        Configuration()
        {
            _t.sub_tables[(std::string)internal].data[(std::string)luaStartNewThreadTime] = 0.01;

#if CBEAM_DEBUG_LOGGING
            _t.sub_tables[(std::string)internal].data[(std::string)logMessages]    = true;
            _t.sub_tables[(std::string)internal].data[(std::string)logReplication] = true;
#else
            _t.sub_tables[(std::string)internal].data[(std::string)logMessages]    = false;
            _t.sub_tables[(std::string)internal].data[(std::string)logReplication] = false;
#endif
        }

        virtual ~Configuration() = default;

        template <typename T>
        T GetInternal(const std::string_view& key) ///< get the given configuration value
        {
            std::lock_guard lock(_mtx);
            return _t.sub_tables[(std::string)internal].get_mapped_value_or_throw<T>((std::string)key);
        }

        LuaTable GetTable() ///< get the whole configuration table, including internal configuration
        {
            std::lock_guard lock(_mtx);
            return _t;
        }

        void SetTable(const LuaTable& t) ///< replace the configuration table, including the internal configuration
        {
            std::lock_guard lock(_mtx);
            _t = t;
        }

        static constexpr std::string_view internal{"internal"};                           ///< the name of the SubTable the contains the list of internal values, like the following
        static constexpr std::string_view luaStartNewThreadTime{"luaStartNewThreadTime"}; ///< stores a double value in seconds that is used to decide after which non-idle time an agent replicates, i. e. creates another hardware thread to distribute work load.
        static constexpr std::string_view logMessages{"logMessages"};                     ///< stores a bool value (default false); if true, all nexuslua messages are logged to "nexuslua.log" in the user folder (see cbeam::filesystem::get_user_data_dir)
        static constexpr std::string_view logReplication{"logReplication"};               ///< stores a bool value (default false); if true, each time an agent is replicated a corresponding log entry is created in file "nexuslua.log" in the user folder (see cbeam::filesystem::get_user_data_dir)

    private:
        LuaTable   _t;
        std::mutex _mtx;
    };
} // namespace nexuslua
