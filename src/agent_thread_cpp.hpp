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
#include "agent_thread.hpp"

#include <filesystem>

namespace nexuslua
{
    class AgentThreadCpp : public AgentThread
    {
    public:
        using message_manager_type = cbeam::concurrency::message_manager<std::shared_ptr<Message>>;

        AgentThreadCpp(const CppHandler& cppHandler, Agent* agent, std::shared_ptr<message_manager_type> message_manager);
        virtual ~AgentThreadCpp();

        AgentThreadCpp(const AgentThreadCpp&)            = delete;
        AgentThreadCpp& operator=(const AgentThreadCpp&) = delete;

    private:
        void handleMessage(std::shared_ptr<Message> message) override;

        CppHandler _cppHandler;
    };
}
