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
#include "lua.hpp"

#include <cbeam/container/thread_safe_set.hpp>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <set>
#include <thread>

namespace nexuslua
{
    class AgentThreadLua
        : public AgentThread
    {
    public:
        using replication          = cbeam::container::thread_safe_set<std::shared_ptr<AgentThreadLua>>;
        using message_manager_type = cbeam::concurrency::message_manager<std::shared_ptr<Message>>;

        AgentThreadLua(const std::filesystem::path&          luaFilePath,
                       const std::string&                    luaCode,
                       Agent*                                agent,
                       std::shared_ptr<message_manager_type> message_manager,
                       std::shared_ptr<Message>              incoming_message = nullptr,
                       std::shared_ptr<replication>          replicated       = nullptr);
        virtual ~AgentThreadLua();

        std::size_t GetReplicatedCount();

        AgentThreadLua(const AgentThreadLua&)            = delete;
        AgentThreadLua& operator=(const AgentThreadLua&) = delete;

    private:
        void        run_lua_script(const std::filesystem::path& luaFilePath, const std::string& luaCode, Agent* agent);
        void        handleMessage(std::shared_ptr<Message> message) override;
        std::string get_instance_description();

        using HandleMessageFunction = std::function<void(std::shared_ptr<Message> message)>;
        static HandleMessageFunction _unreplicatedHandleMessage;

        Lua                         _lua;
        const std::filesystem::path _luaFilePath;
        const std::string           _luaCode;
        const bool                  _isReplicated;

        std::shared_ptr<replication> _replicated;

        std::chrono::time_point<std::chrono::high_resolution_clock> _timeOfLastMessage;
        std::mutex                                                  _mtxTimeOfLastMessage;
    };
}
