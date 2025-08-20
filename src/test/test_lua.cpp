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

#include <cbeam/container/xpod.hpp>

#include "lua.h"

namespace nexuslua
{
    TEST(NexusLuaCoreValueTest, TypeCheck)
    {
        cbeam::container::xpod::type test;

        using IntegerType = std::remove_reference<decltype(std::get<cbeam::container::xpod::type_index::integer>(test))>::type;
        using ValueType   = std::remove_reference<decltype(std::get<cbeam::container::xpod::type_index::number>(test))>::type;

        EXPECT_TRUE((std::is_same<IntegerType, lua_Integer>::value))
            << "The type type_index::integer of cbeam::value does not match lua_Integer.";

        EXPECT_TRUE((std::is_same<ValueType, lua_Number>::value))
            << "The type type_index::number of cbeam::value does not match lua_Number.";
    }
}
