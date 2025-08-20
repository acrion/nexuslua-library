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

#include "agent_thread_cpp.hpp"
#include "agents.hpp"
#include "message_counter.hpp"

#include <cbeam/logging/log_manager.hpp>
#include <cbeam/serialization/xpod.hpp>

namespace nexuslua
{
    AgentThreadCpp::AgentThreadCpp(const CppHandler& cppHandler, Agent* agent, std::shared_ptr<message_manager_type> message_manager)
        : AgentThread{agent, message_manager}
        , _cppHandler{cppHandler}
    {
        CBEAM_LOG_DEBUG("            New agent '" + agent->GetName() + "' for C++ handler");
    }

    AgentThreadCpp::~AgentThreadCpp()
    {
    }

    void AgentThreadCpp::handleMessage(std::shared_ptr<Message> incoming_message)
    {
        _cppHandler(incoming_message);
        message_counter::get()->decrease();
    }
}
