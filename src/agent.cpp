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

#include <cbeam/container/find.hpp>

#include <cbeam/serialization/xpod.hpp>

#include "agent.hpp"

#include "config.hpp"
#include "configuration.hpp"
#include "thread_pool.hpp"

#include <cbeam/lifecycle/item_registry.hpp>
#include <cbeam/lifecycle/singleton.hpp>
#include <cbeam/logging/log_manager.hpp>
#include <cbeam/serialization/xpod.hpp>

#include <cassert>

#include <mutex>

namespace nexuslua
{
    using namespace std::string_literals;

    struct Agent::Impl
    {
        std::weak_ptr<agents> _agents;
        int                   _id{-1};
        AgentType             _agentType{AgentType::Undefined};
        Configuration         _configuration;

        std::shared_ptr<cbeam::lifecycle::item_registry> _agent_registry{cbeam::lifecycle::singleton<cbeam::lifecycle::item_registry>::get("agent_registry")};
    };

    Agent::Agent(const std::shared_ptr<agents>& agent_group)
        : _impl{std::make_unique<Impl>()}
    {
        _impl->_agents = agent_group;
    }

    Agent::~Agent()
    {
        if (_impl->_id >= 0)
        {
            CBEAM_LOG_DEBUG("        Destroying agent " + std::to_string(_impl->_id) + ".");
            _impl->_agent_registry->deregister_item(_impl->_id);
        }
    }

    Configuration& Agent::GetConfiguration()
    {
        return _impl->_configuration;
    }

    std::shared_ptr<agents> Agent::GetAgents()
    {
        if (auto agents_ptr = _impl->_agents.lock())
        {
            return agents_ptr;
        }

        throw std::runtime_error("Agent's manager (agents) is no longer available.");
    }

    std::filesystem::path Agent::GetInstallFolder() const /// \todo change return type to std::filesystem::path
    {
        return {}; // default is an agent that has not been installed (i.e. a "plugin", see class AgentPlugin), but created during runtime
    }

    std::filesystem::path Agent::GetPersistentFolder() const /// \todo change return type to std::filesystem::path
    {
        return {}; // only set for plugins (class AgentPlugin)
    }

    std::string Agent::GetVersionOnline() const
    {
        return ""s; // only set for plugins (class AgentPlugin)
    }

    std::string Agent::GetVersionInstalled() const
    {
        return ""s; // only set for plugins (class AgentPlugin)
    }

    bool Agent::IsFreeware() const
    {
        return false; // only set for plugins (class AgentPlugin)
    }

    std::string Agent::GetUrlHelp() const
    {
        return ""s; // only set for plugins (class AgentPlugin)
    }

    std::string Agent::GetUrlDownload() const
    {
        return ""s; // only set for plugins (class AgentPlugin)
    }

    std::string Agent::GetUrlLicense() const
    {
        return ""s; // only set for plugins (class AgentPlugin)
    }

    std::string Agent::GetUrlPurchase() const
    {
        return ""s; // only set for plugins (class AgentPlugin)
    }

    std::string Agent::GetLicensee() const
    {
        return ""s; // only set for plugins (class AgentPlugin)
    }

    const std::map<std::string, AgentMessage>& Agent::GetMessages() const
    {
        return _messages;
    }

    const AgentMessage& Agent::GetMessage(const std::string& messageName) const
    {
        auto messageIt = _messages.find(messageName);

        if (messageIt == _messages.end())
        {
            throw std::runtime_error("Agent::GetMessages: message '" + messageName + "' is unknown in agent '" + GetName() + "'.");
        }

        return messageIt->second;
    }

    void Agent::AddMessage(const std::string& messageName, const LuaTable::nested_tables& parameterDescriptions, const std::string& displayName, const std::string& description, const std::string& icon)
    {
        _messages.emplace(messageName, AgentMessage(_impl->_id, _impl->_agentType, GetName(), messageName, parameterDescriptions, displayName, description, icon));
    }

    void Agent::Start(const std::filesystem::path& luaPath, const std::string& luaCode)
    {
        _impl->_id        = (int)_impl->_agent_registry->register_item();
        _impl->_agentType = AgentType::Lua;
        CBEAM_LOG_DEBUG("Agent::Start: id==" + std::to_string(_impl->_id) + " " + luaPath.string());
        auto thread_pool_ptr = ThreadPool::Get(_impl->_agents);
        if (thread_pool_ptr)
        {
            thread_pool_ptr->StartThread(luaPath, luaCode, this);
        }
        else
        {
            CBEAM_LOG("Did not (re-)start Lua agent because shutdown had been initiated.");
        }
    }

    void Agent::Start(const CppHandler& cppHandler)
    {
        _impl->_id        = (int)_impl->_agent_registry->register_item();
        _impl->_agentType = AgentType::Cpp;
        CBEAM_LOG_DEBUG("Agent::Start: id==" + std::to_string(_impl->_id) + " C++");
        auto thread_pool_ptr = ThreadPool::Get(_impl->_agents);
        if (thread_pool_ptr)
        {
            thread_pool_ptr->StartThread(cppHandler, this);
        }
        else
        {
            CBEAM_LOG("Did not (re-)start C++ agent because shutdown had been initiated.");
        }
    }

    int Agent::GetId() const
    {
        if (_impl->_id == -1)
        {
            throw std::runtime_error("Agent::GetId(): Inherited class needs to call Start() prior calling GetId()");
        }

        return _impl->_id;
    }
}
