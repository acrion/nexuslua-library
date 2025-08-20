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

#include <cbeam/convert/xpod.hpp>

#include <cbeam/convert/nested_map.hpp>

#include <cbeam/random/generators.hpp>

#include <nexuslua/message.hpp>

#include <memory>
#include <random>

namespace nexuslua
{
    using namespace std::string_literals;

    class MessageCloneTest : public ::testing::Test
    {
    protected:
        nexuslua::LuaTable GenerateRandomLuaTable(int depth, int strLen, unsigned seed)
        {
            std::mt19937 gen(seed);

            auto randomValue = [&gen, &strLen]() -> cbeam::container::xpod::type
            {
                std::uniform_int_distribution<int> typeDist(0, std::variant_size<cbeam::container::xpod::type>() - 1);
                switch (typeDist(gen))
                {
                case cbeam::container::xpod::type_index::integer:
                    return std::uniform_int_distribution<long long>(0, 100)(gen);
                case cbeam::container::xpod::type_index::number:
                    return std::uniform_real_distribution<double>(0.0, 100.0)(gen);
                case cbeam::container::xpod::type_index::boolean:
                    return std::uniform_int_distribution<int>(0, 1)(gen) == 1;
                case cbeam::container::xpod::type_index::pointer:
                    return cbeam::memory::pointer(std::to_string(gen()));
                case cbeam::container::xpod::type_index::string:
                    return cbeam::random::random_string(strLen, gen);
                }
                return 0LL;
            };

            std::function<nexuslua::LuaTable(int)> generateNestedTable = [&](int currentDepth) -> nexuslua::LuaTable
            {
                nexuslua::LuaTable table;
                if (currentDepth > 0)
                {
                    table.data[randomValue()]       = randomValue();
                    table.sub_tables[randomValue()] = generateNestedTable(currentDepth - 1);
                }
                return table;
            };

            return generateNestedTable(depth);
        }
    };

    TEST_F(MessageCloneTest, CloneMessageTest)
    {
        CBEAM_LOG_DEBUG("MessageCloneTest");

        constexpr int testCount  = 1000;
        constexpr int strLen     = 16;
        constexpr int tableDepth = 5;

        bool                         failure = false;
        std::shared_ptr<Message> failedMessageOriginal;
        std::shared_ptr<Message> failedMessageClone;

#pragma omp parallel for shared(failure)
        for (int i = 0; i < testCount; ++i)
        {
            if (!failure)
            {
                auto originalMessage        = std::make_shared<Message>(0);
                originalMessage->parameters = GenerateRandomLuaTable(tableDepth, strLen, i);

                auto clonedMessage = originalMessage->clone();
                originalMessage.reset();

                auto regeneratedOriginalMessage        = std::make_shared<Message>(0);
                regeneratedOriginalMessage->parameters = GenerateRandomLuaTable(tableDepth, strLen, i);

                if (regeneratedOriginalMessage->parameters != clonedMessage->parameters)
                {
#pragma omp critical
                    {
                        failure               = true;
                        failedMessageOriginal = regeneratedOriginalMessage;
                        failedMessageClone    = std::dynamic_pointer_cast<Message>(clonedMessage);
                    }
                }
            }
        }

        if (failure)
        {
            std::cout << "Message did not clone correctly. Original:" << std::endl
                      << cbeam::convert::to_string(failedMessageOriginal->parameters)
                      << std::endl
                      << "Clone:" << std::endl
                      << cbeam::convert::to_string(failedMessageClone->parameters)
                      << std::endl;
            ASSERT_FALSE(failure);
        }
    }
}
