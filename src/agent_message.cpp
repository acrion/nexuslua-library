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

#include "agent_message.hpp"

#include "agent_thread_lua.hpp"
#include "lua.hpp"
#include "message_counter.hpp"
#include "platform_specific.hpp"
#include "thread_pool.hpp"

#include "lua_table.hpp"

#include <cbeam/convert/xpod.hpp>
#include <cbeam/filesystem/path.hpp>

#include <sstream>

using namespace nexuslua;

AgentMessage::AgentMessage(const int agentN, const AgentType& agentType, const std::string& agentName, const std::string& messageName, const LuaTable::nested_tables& parameterDescriptions, const std::string& displayName, const std::string& description, const std::string& icon)
    : _agentN(agentN)
    , _agentType(agentType)
    , _agentName(agentName)
    , _messageName(messageName)
    , _parameterDescriptions(parameterDescriptions)
    , _displayName(displayName.empty() ? messageName : displayName)
    , _description(description.empty() ? _displayName : description)
    , _svgIcon(icon)
{
    if (messageName.empty())
    {
        throw std::runtime_error("nexuslua::AgentMessage: empty message name");
    }

    if (agentType == AgentType::Undefined)
    {
        throw std::logic_error("nexuslua::AgentMessage: Undefined agent type for agent " + agentName);
    }
}

AgentMessage::AgentMessage(const int agentN, const AgentType& agentType, const std::string& agentName, const std::string& messageName)
    : _agentN{agentN}
    , _agentType(agentType)
    , _agentName{agentName}
    , _messageName{messageName}
    , _displayName{messageName}
    , _description{messageName}
{
    if (messageName.empty())
    {
        throw std::runtime_error("nexuslua::AgentMessage: empty message name");
    }

    if (agentType == AgentType::Undefined)
    {
        throw std::logic_error("nexuslua::AgentMessage: Undefined agent type for agent " + agentName);
    }
}

void AgentMessage::Validate(const LuaTable& parameterValues) const
{
    for (const auto& parameterDescriptionData : _parameterDescriptions)
    {
        if (parameterValues.data.find(parameterDescriptionData.first) == parameterValues.data.end()
            && parameterValues.sub_tables.find(parameterDescriptionData.first) == parameterValues.sub_tables.end())
        {
            throw std::runtime_error("nexuslua::AgentMessage '" + _displayName + "': Missing parameter value for " + cbeam::convert::to_string(parameterDescriptionData.first));
        }
    }
}

void AgentMessage::Send(const LuaTable& parameterValues) const
{
    message_counter::get()->increase();

    LuaTable parameterValuesPlusDefault = AddDefaultParameterValues(parameterValues);

    Validate(parameterValuesPlusDefault);

    auto thread_pool = ThreadPool::Get();
    if (thread_pool)
    {
        thread_pool->SendMessage(std::make_shared<Message>(_agentN, _messageName, parameterValuesPlusDefault));
    }
    else
    {
        CBEAM_LOG("Skipped message '" + _messageName + "' because shutdown had been initiated");
    }
}

LuaTable::nested_tables AgentMessage::GetDescriptionsOfUnsetParameters(const LuaTable& parameterValues) const
{
    LuaTable::nested_tables unsetParameterDescriptions;

    for (const auto& p : _parameterDescriptions)
    {
        if (parameterValues.data.find(p.first) == parameterValues.data.end() && parameterValues.sub_tables.find(p.first) == parameterValues.sub_tables.end())
        {
            unsetParameterDescriptions[p.first] = p.second;
        }
    }

    return unsetParameterDescriptions;
}

LuaTable AgentMessage::AddDefaultParameterValues(const LuaTable& parameterValues) const
{
    LuaTable result = parameterValues;

    for (const auto& p : _parameterDescriptions)
    {
        const auto& resultData = result.data.find(p.first);
        if (resultData == result.data.end())
        {
            const auto& parameterData = p.second.data.find("default");

            if (parameterData != p.second.data.end())
            {
                result.data[p.first] = parameterData->second;
            }
        }

        const auto& resultSubTable = result.sub_tables.find(p.first);
        if (resultSubTable == result.sub_tables.end())
        {
            const auto& parameterSubTable = p.second.sub_tables.find("default");

            if (parameterSubTable != p.second.sub_tables.end())
            {
                result.sub_tables[p.first] = parameterSubTable->second;
            }
        }
    }

    return result;
}

std::string AgentMessage::GetAgentName() const { return _agentName; }

AgentType AgentMessage::GetAgentType() const { return _agentType; }

std::string AgentMessage::GetMessageName() const { return _messageName; }

std::string AgentMessage::GetDisplayName() const { return _displayName; }

std::string AgentMessage::GetDescription() const { return _description; }

LuaTable::nested_tables AgentMessage::GetParameterDescriptions() const { return _parameterDescriptions; }

std::string AgentMessage::GetIconPath() const { return _svgIcon; }
