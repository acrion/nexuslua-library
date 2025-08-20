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

#include "agent_thread_base.hpp"

#include <cbeam/convert/xpod.hpp>

#include <cbeam/concurrency/message_manager.hpp>
#include <cbeam/convert/nested_map.hpp>

#include "agent_thread.hpp"

#include "configuration.hpp"

#include <cbeam/logging/log_manager.hpp>
#include <cbeam/serialization/xpod.hpp>

namespace nexuslua
{
    class AgentThread : public agent_thread_base
    {
    public:
        using message_manager_type = cbeam::concurrency::message_manager<std::shared_ptr<Message>>;

        AgentThread(Agent* agent, const std::shared_ptr<message_manager_type> message_manager, const std::string& threadName = {})
            : agent_thread_base(agent, threadName)
            , _message_manager{message_manager}
        {
        }

        virtual ~AgentThread()
        {
            _message_manager->dispose(_agent->GetId());
        }

        void addHandler()
        {
            _message_manager->add_handler(_agent->GetId(), [this](std::shared_ptr<Message> message)
                                          { handleMessage(message); },
                                          nullptr,
                                          nullptr,
                                          _tName,
                                          cbeam::concurrency::message_manager<std::shared_ptr<Message>>::order_type::FIFO);

            if (_agent->GetConfiguration().GetInternal<bool>(Configuration::logMessages))
            {
                //                std::string agent_name = GetAgent()->GetName();
                //                bool        agent_name_empty  = agent_name.empty();
                //                size_t      agent_name_length = agent_name.length();
                //                assert(agent_name_empty || agent_name_length <= 1024);

                _message_manager->set_logger(_agent->GetId(), [=](std::size_t receiver, std::shared_ptr<Message> msg, bool sending)
                                             {
//                    TODO agent_name is sporadically corrupted after being copied into this Lambda (above assert is okay when below is not). Best to reproduce with demo5_fast.lua
//                    bool agent_name_empty = agent_name.empty();
//                    size_t agent_name_length = agent_name.length();
//                    assert(agent_name_empty || agent_name_length <= 1024);
//                    CBEAM_LOG("Agent " + agent_name + ": Message " + msg->name + " to handler" + std::to_string(receiver)+ " was " + (sending ? "sent" : "received") + " with parameters\n" + cbeam::convert::to_string(msg->parameters)); });
                      CBEAM_LOG("Message " + msg->name + " to handler" + std::to_string(receiver) + " was " + (sending ? "sent" : "received") + " with parameters\n" + cbeam::convert::to_string(msg->parameters)); });
            }
        }

        AgentThread(const AgentThread&)            = delete;
        AgentThread& operator=(const AgentThread&) = delete;

    protected:
        std::shared_ptr<message_manager_type> _message_manager;

    private:
        virtual void handleMessage(std::shared_ptr<Message> message) = 0;
    };
}