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

#include "agent.hpp"
#include "agent_message.hpp"
#include "cpp_handler.hpp"

#include "nexuslua_export.h"

#include <map>
#include <memory>
#include <string>

namespace nexuslua
{
    class agents;

    class NEXUSLUA_EXPORT AgentCpp : public Agent
    {
    public:
        explicit AgentCpp(const std::shared_ptr<agents> agent_group, const std::string& name);
        virtual ~AgentCpp();
        void Start(const CppHandler& cppHandler);
        using Agent::AddMessage;
        void        AddMessage(const std::string& messageName);
        std::string GetName() const override;

    private:
        const std::string _name;
    };
}