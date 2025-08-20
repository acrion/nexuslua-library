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

#include "lua_table.hpp"

#include "nexuslua_export.h"

#include <atomic>
#include <memory>
#include <string>

namespace nexuslua
{
    class Lua;

    enum class AgentType
    {
        Undefined = -1,
        Lua       = 0,
        Cpp       = 1
    };

    /// \brief This class describes a message that can be sent via nexuslua \ref send or AgentMessage::Send.
    /// \details An instance of this class can be created via \ref addmessage or nexuslua::agents::AddMessageForCppAgent. Please note that the actual data that is sent is composed of AgentMessage::GetMessageName() along with the cbeam::serialization::serialized_object that is passed to the AgentMessage::Send method.
    class NEXUSLUA_EXPORT AgentMessage
    {
    public:
        AgentType               GetAgentType() const;                                                    ///< return the type of the agent that accepts this message (Lua or C++)
        std::string             GetAgentName() const;                                                    ///< return the name of the agent that accepts this message
        std::string             GetMessageName() const;                                                  ///< this message name is used by AgentMessage::Send
        std::string             GetDisplayName() const;                                                  ///< return a display name of this message (may be empty, usually set for agents that are used as plugins, see PluginsOnline::Get)
        std::string             GetDescription() const;                                                  ///< return a description of this message (may be empty, usually set for agents that are used as plugins, see PluginsOnline::Get)
        LuaTable::nested_tables GetParameterDescriptions() const;                                        ///< return descriptions for each of the parameters of this message (may be empty, usually set for agents that are used as plugins, see PluginsOnline::Get)
        LuaTable::nested_tables GetDescriptionsOfUnsetParameters(const LuaTable& parameterValues) const; ///< convenience method. Returns only those descriptions of parameters that are not part of the given parameter values  (may be empty, usually set for agents that are used as plugins, see PluginsOnline::Get)
        std::string             GetIconPath() const;                                                     ///< return the path to an icon that can be shown in a graphical user interface for this message (may be empty, usually set for agents that are used as plugins, see PluginsOnline::Get)
        void                    Send(const LuaTable& parameters) const;                                  ///< completes values that are missing in parameters based on GetParameterDescriptions() with their default values and sends the message (named GetMessageName()).

    private:
        friend class Agent;
        friend class AgentCpp;

        AgentMessage(const int agentN, const AgentType& agentType, const std::string& agentName, const std::string& messageName, const LuaTable::nested_tables& parameterDescriptions, const std::string& displayName, const std::string& description, const std::string& icon);
        AgentMessage(const int agentN, const AgentType& agentType, const std::string& agentName, const std::string& messageName);

        int                     _agentN;
        AgentType               _agentType;
        std::string             _agentName;
        std::string             _messageName;
        LuaTable::nested_tables _parameterDescriptions;
        std::string             _displayName;
        std::string             _description;
        std::string             _svgIcon;
        std::shared_ptr<Lua>    _lua;

        void     Validate(const LuaTable& parameterValues) const;
        LuaTable AddDefaultParameterValues(const LuaTable& parameterValues) const;
    };
}
