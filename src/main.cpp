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

#include "description.hpp"

#include <cbeam/logging/log_manager.hpp>
#include <cbeam/platform/system_folders.hpp>

namespace nexuslua
{
    struct Initializer
    {
        Initializer()
        {
            cbeam::logging::log_manager::create_logfile(cbeam::filesystem::get_user_cache_dir() / description::GetProductName() / (description::GetProductName() + ".log"));

            // TODO
//            std::setlocale(LC_ALL, "C");                 // for C and C++ where synced with stdio
//            std::locale::global(std::locale::classic()); // for C++
        }
    } initializer;
}
