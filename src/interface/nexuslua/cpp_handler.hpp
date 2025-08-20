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

#include <functional>
#include <memory>

namespace nexuslua
{
    struct Message;

    /// \brief the signature of a function that is called whenever a C++ nexuslua::agent receives a message
    /// \details The callback function is registered via nexuslua::agents::Add. All message names
    /// that will be sent must first be registered via nexuslua::agents::AddMessageForCppAgent.
    typedef std::function<void(std::shared_ptr<Message>)> CppHandler;
}
