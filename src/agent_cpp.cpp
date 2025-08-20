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

#include "agent_cpp.hpp"

#include "agent.hpp"
#include "agents.hpp"

#include <cbeam/serialization/xpod.hpp>

namespace nexuslua
{
    AgentCpp::AgentCpp(const std::shared_ptr<agents> agent_group, const std::string& name)
        : Agent{agent_group}
        , _name{name}
    {
    }

    AgentCpp::~AgentCpp()
    {
        CBEAM_LOG_DEBUG("Destroying AgentCpp '" + _name + "'");
    }

    void AgentCpp::Start(const CppHandler& cppHandler)
    {
        Agent::Start(cppHandler);
    }

    void AgentCpp::AddMessage(const std::string& messageName)
    {
        auto messageIt = _messages.find(messageName);

        if (messageIt != _messages.end())
        {
            throw std::runtime_error("AgentCpp::AddMessage: message '" + messageName + "' is already registered in agent '" + _name + "'.");
        }

        _messages.emplace(messageName, AgentMessage(GetId(), AgentType::Cpp, _name, messageName));
    }

    std::string AgentCpp::GetName() const
    {
        return _name;
    }
}
