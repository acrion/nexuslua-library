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

#include <gtest/gtest.h>

#include <cbeam/container/find.hpp>

#include "nexuslua/configuration.hpp"

namespace nexuslua
{
    using namespace std::string_literals;

    TEST(ConfigurationTest, testLuaStartNewThreadTime)
    {
        Configuration configuration;
        auto          luaStartNewThreadTime = configuration.GetInternal<double>(Configuration::luaStartNewThreadTime);
        EXPECT_EQ(luaStartNewThreadTime, 0.01);
    }

    TEST(ConfigurationTest, testUserConfig)
    {
        Configuration     configuration;
        const std::string testEntry = "my entry"s;
        const long long   testValue = 42;

        auto config            = configuration.GetTable();
        config.data[testEntry] = testValue;
        configuration.SetTable(config);

        auto currentValue = configuration.GetTable().get_mapped_value_or_default<long long>(testEntry);

        EXPECT_EQ(currentValue, testValue);
    }
}
