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

#include "agent_thread_lua.hpp"

#include "agent_message.hpp"
#include "agents.hpp"
#include "configuration.hpp"
#include "lua_extension.hpp"
#include "message_counter.hpp"
#include "platform_specific.hpp"

#include <cbeam/convert/xpod.hpp>

#include <cbeam/container/find.hpp>
#include <cbeam/convert/nested_map.hpp>
#include <cbeam/logging/log_manager.hpp>

#include "lua_table.hpp"

using namespace std::string_literals;

namespace nexuslua
{
    AgentThreadLua::AgentThreadLua(
        const std::filesystem::path&                                                        luaFilePath,
        const std::string&                                                                  luaCode,
        Agent*                                                                              agent,
        std::shared_ptr<message_manager_type>                                               message_manager,
        std::shared_ptr<Message>                                                            incoming_message,
        std::shared_ptr<cbeam::container::thread_safe_set<std::shared_ptr<AgentThreadLua>>> replicated)
        : AgentThread{agent, message_manager, "h_" + luaFilePath.stem().string()}
        , _lua{agent}
        , _luaFilePath{luaFilePath}
        , _luaCode{luaCode}
        , _isReplicated{incoming_message != nullptr}
        , _replicated{replicated ? replicated : std::make_shared<cbeam::container::thread_safe_set<std::shared_ptr<AgentThreadLua>>>()}
    {
        assert((replicated != nullptr) == (incoming_message != nullptr));

        std::string threadName = _isReplicated ? "RL" : "L";
        if (!luaCode.empty())
        {
            threadName += "C";
        }
        threadName += agent->GetName();
        cbeam::concurrency::set_thread_name(threadName.c_str());

        if (_luaCode.empty())
        {
            CBEAM_LOG_DEBUG("            " + get_instance_description() + ": New agent '" + threadName + "' for Lua script '" + _luaFilePath.string() + "', replicated = " + std::to_string(_isReplicated));
        }
        else
        {
            CBEAM_LOG_DEBUG("            " + get_instance_description() + ": New agent '" + threadName + "' for Lua code contained in script '" + _luaFilePath.string() + "', replicated = " + std::to_string(_isReplicated));
        }

        run_lua_script(luaFilePath, luaCode, agent);

        CBEAM_LOG_DEBUG("            " + get_instance_description() + ": adding handler");

        if (incoming_message)
        {
            auto modified_message = incoming_message->clone();
            modified_message->parameters.data.erase("threads");
            handleMessage(modified_message);
        }
    }

    AgentThreadLua::~AgentThreadLua()
    {
        if (!_isReplicated)
        {
            _replicated->clear();
        }
    }

    std::string AgentThreadLua::get_instance_description()
    {
        static std::string description = "AgentThreadLua<MessageToAgent<" + std::to_string(GetAgent()->GetId()) + ">> ('" + AgentThread::GetAgent()->GetName() + "')";
        return description;
    }

    void AgentThreadLua::run_lua_script(const std::filesystem::path& luaFilePath, const std::string& luaCode, Agent* agent)
    {
        LuaExtension::StoreAgentOfLuaState(_lua.GetState(), agent, luaFilePath.string(), _isReplicated);

        try
        {
            if (luaCode.empty())
            {
                _lua.Run(luaFilePath);
            }
            else
            {
                _lua.Run(luaCode, luaFilePath);
            }
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error(get_instance_description() + ": " + (luaFilePath.empty() ? ex.what() : "Exception during execution of " + luaFilePath.string() + ": " + ex.what()));
        }
        catch (...)
        {
            throw std::runtime_error(get_instance_description() + ": " + "Unknown exception during execution of Lua code " + luaFilePath.string());
        }
    }

    void AgentThreadLua::handleMessage(std::shared_ptr<Message> incoming_message)
    {
        const auto currentTime = std::chrono::high_resolution_clock::now();

        bool idle;
        {
            std::lock_guard lock(_mtxTimeOfLastMessage);
            idle = std::chrono::duration<double>(currentTime - _timeOfLastMessage).count() > AgentThread::GetAgent()->GetConfiguration().template GetInternal<double>(Configuration::luaStartNewThreadTime);
        }

        bool handled = false;
        if (!idle)
        {
            auto replicate = incoming_message->parameters.data.find("threads");
            if (replicate != incoming_message->parameters.data.end())
            {
                const std::size_t requested_threads = cbeam::container::get_value_or_default<cbeam::container::xpod::type_index::integer>(
                    replicate->second);

                auto lock = _replicated->get_lock_guard();
                if (_replicated->size() + 1 < requested_threads)
                {
                    auto replicated_thread = std::make_shared<AgentThreadLua>(
                        _luaFilePath,
                        _luaCode,
                        AgentThread::GetAgent(),
                        _message_manager,
                        incoming_message,
                        _replicated);

                    replicated_thread->addHandler();
                    _replicated->emplace(replicated_thread);

                    handled = true;

                    auto agent = AgentThread::GetAgent();

                    if (agent->GetConfiguration().template GetInternal<bool>(Configuration::logReplication))
                    {
                        const std::string logPart1 = _replicated->size() == 1 ? "Agent '" + agent->GetName() + "' is"
                                                                              : "All agents '" + agent->GetName() + "' are";
                        const std::string logPart2 = _luaCode.empty() ? ""
                                                                      : "code contained in ";

                        CBEAM_LOG(logPart1 + " busy => replicating to " + std::to_string(_replicated->size() + 1) + " threads to process incoming message '" + incoming_message->name + "' (Lua " + logPart2 + "script '" + _luaFilePath.string() + "')");
                    }
                }
                else
                {
                    CBEAM_LOG_DEBUG("            " + get_instance_description() + ": All " + std::to_string(requested_threads) + " replicated threads for " + incoming_message->name + " are busy, Lua script '" + _luaFilePath.string() + "'");
                }
            }
        }

        if (!handled)
        {
#ifdef CBEAM_DEBUG_LOGGING
            {
                std::stringstream log;
                log << "            " + get_instance_description() + ": handling message '" << incoming_message->name << "' for Lua " << (_luaCode.empty() ? "script " : "code contained in '") << _luaFilePath.string() << "'";

                if (GetAgent()->GetConfiguration().GetInternal<bool>(Configuration::logMessages))
                {
                    // only log the message content if the generic logging is active, too
                    log << " with parameters:" << std::endl
                        << cbeam::convert::to_string(incoming_message->parameters);
                }

                CBEAM_LOG_DEBUG(log.str());
            }
#endif
            // The actual message processing code is in a try-catch block to handle any exceptions
            // that might be thrown.
            try
            {
                LuaTable result = _lua.RunPlugin(*incoming_message);

                const std::string& reply_to_agent = incoming_message->parameters.GetReplyToAgentNameOrEmpty();

                if (!reply_to_agent.empty())
                {
                    const std::string& reply_to_message = incoming_message->parameters.GetReplyToMessageNameOrEmpty();

                    if (!reply_to_message.empty())
                    {
                        const AgentMessage& message = AgentThread::GetAgent()->GetAgents()->GetMessage(reply_to_agent, reply_to_message);

                        result.SetOriginalMessage(incoming_message);
                        result.merge(incoming_message->parameters.GetTableToMergeWhenReplyingOrEmpty());

                        message.Send(result);
                    }
                }
                message_counter::get()->decrease();
            }
            catch (const std::exception& ex)
            {
                CBEAM_LOG("            " + get_instance_description() + ": handleMessage: "s + ex.what());
            }
            catch (...)
            {
                CBEAM_LOG("            " + get_instance_description() + ": handleMessage: unknown exception");
            }
        }

        std::lock_guard lock(_mtxTimeOfLastMessage);
        _timeOfLastMessage = std::chrono::high_resolution_clock::now();
    }

    std::size_t AgentThreadLua::GetReplicatedCount()
    {
        return _replicated->size();
    }
}
