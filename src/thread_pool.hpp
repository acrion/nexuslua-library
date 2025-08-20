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
#include "agent_thread_base.hpp"
#include "agent_thread_cpp.hpp"
#include "agent_thread_lua.hpp"
#include "agents.hpp"
#include "config.hpp"

#include "nexuslua_export.h"

#include <cbeam/lifecycle/item_registry.hpp>
#include <cbeam/lifecycle/singleton.hpp>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>

namespace nexuslua
{
    class ThreadPool
    {
    public:
        ThreadPool()
            : _message_manager{std::make_shared<message_manager_type>()}
        {
        }
        virtual ~ThreadPool()
        {
            _agentThreads.clear();
            if (auto locked = _agent_list.lock())
            {
                locked->DeleteAgents();
            }
            _message_manager.reset();
        }

        static std::shared_ptr<ThreadPool> Get(std::weak_ptr<agents> agent_list)
        {
            {
                auto current_locked = _agent_list.lock();
                auto new_locked     = agent_list.lock();

                if (current_locked && new_locked && current_locked.get() != new_locked.get())
                {
                    throw std::runtime_error("Internal error: nexuslua::ThreadPool doesn't have same agent list");
                }
            }

            _agent_list = agent_list;
            return cbeam::lifecycle::singleton<ThreadPool>::get("nexuslua::thread_pool");
        }

        static std::shared_ptr<ThreadPool> Get()
        {
            if (_agent_list.expired())
            {
                throw std::runtime_error("ThreadPool is not initialized. You need to call first the Get method that accepts the agent list");
            }

            return cbeam::lifecycle::singleton<ThreadPool>::get("nexuslua::thread_pool");
        }

        void StartThread(const std::filesystem::path& luaFilePath, const std::string& luaCode, Agent* agent)
        {
            _agentThreads[agent->GetId()] =
                std::make_unique<AgentThreadLua>(luaFilePath, luaCode, agent, _message_manager);

            _agentThreads[agent->GetId()]->addHandler(); // this starts the (handler) thread
        }

        void StartThread(const CppHandler& cppHandler, Agent* agent)
        {
            _agentThreads[agent->GetId()] =
                std::make_unique<AgentThreadCpp>(cppHandler, agent, _message_manager);

            _agentThreads[agent->GetId()]->addHandler(); // this starts the (handler) thread
        }

        void SendMessage(std::shared_ptr<Message> message)
        {
            static constexpr std::string_view queueKey{"queue"};

            const std::size_t queue_value = (std::size_t)message->parameters.template get_mapped_value_or_default<cbeam::container::xpod::type_index::integer>(queueKey.data());
            // const std::size_t receiver    = message->parameters.RequestsUnreplicatedReceiver() ? 0 : -1;

            _message_manager->send_message(message->agent_n, message, queue_value); // TODO
        }

    private:
        ThreadPool(const ThreadPool&)            = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        using message_manager_type = cbeam::concurrency::message_manager<std::shared_ptr<Message>>;

        std::shared_ptr<message_manager_type>                     _message_manager;
        std::map<std::size_t, std::unique_ptr<agent_thread_base>> _agentThreads;
        inline static std::weak_ptr<agents>                       _agent_list;
    };
}
