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

#include "nexuslua/description.hpp"
#include "nexuslua/utility.hpp"

#include <cbeam/filesystem/io.hpp>
#include <cbeam/filesystem/path.hpp>
#include <cbeam/logging/log_manager.hpp>
#include <cbeam/platform/runtime.hpp>

#include <filesystem>

namespace nexuslua
{
    TEST(nexusluaTest, unzip)
    {
        const std::filesystem::path binary_dir = std::filesystem::path(cbeam::platform::get_path_to_runtime_binary()).parent_path();
        const std::filesystem::path input_dir  = binary_dir / "nexuslua-library-testing-resources"; // copied by CMakeLists.txt from src/testing-resources
        const std::filesystem::path output_dir = binary_dir / "nexuslua-library-testing-temp";
        CBEAM_LOG("nexusluaTest::unzip: input_dir:  " + input_dir.string());
        CBEAM_LOG("nexusluaTest::unzip: output_dir: " + output_dir.string());

        cbeam::filesystem::path(output_dir).create_directory(true);

        utility::Zip(input_dir / "test", output_dir / "test.zip");
        EXPECT_EQ(std::filesystem::exists(output_dir / "test.zip"), true);

        utility::Unzip(output_dir / "test.zip", output_dir / "test2");

        EXPECT_EQ(std::filesystem::exists(output_dir / "test2" / "test.txt"), true);
        EXPECT_EQ(std::filesystem::exists(output_dir / "test2" / "subfolder" / "test2.txt"), true);
        EXPECT_EQ(cbeam::filesystem::read_file(input_dir / "test" / "test.txt"), cbeam::filesystem::read_file(output_dir / "test2" / "test.txt"));
        EXPECT_EQ(cbeam::filesystem::read_file(input_dir / "test" / "subfolder" / "test2.txt"), cbeam::filesystem::read_file(output_dir / "test2" / "subfolder" / "test2.txt"));
    }
}
