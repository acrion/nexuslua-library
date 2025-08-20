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
#include "message.hpp"

namespace nexuslua
{
    class agent_thread_base
    {
    public:
        agent_thread_base(Agent* agent, const std::string& threadName = {})
            : _agent{agent}
            , _tName{threadName.empty() ? "h_" + agent->GetName() : threadName}
        {
            CBEAM_LOG_DEBUG("                Creating agent_thread_base " + std::to_string(agent->GetId()) + " ('" + agent->GetName() + "')");
        }

        virtual ~agent_thread_base()
        {
        }

        Agent* GetAgent() const
        {
            return _agent;
        }

        virtual void addHandler()                              = 0;
        agent_thread_base(const agent_thread_base&)            = delete;
        agent_thread_base& operator=(const agent_thread_base&) = delete;

    protected:
        Agent*            _agent;
        const std::string _tName;
    };
}