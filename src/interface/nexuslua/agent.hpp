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

#include "agent_message.hpp"

#include "lua_table.hpp"

#include "agents.hpp"
#include "cpp_handler.hpp"
#include "message.hpp"

#include "nexuslua_export.h"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

/// \namespace nexuslua
/// \brief The nexuslua library is implemented inside this namespace.
namespace nexuslua
{
    class Agent;
    class Configuration;

    namespace LuaExtension
    {
        void AddMessage(Agent* agent, const std::string& luaPath, const std::string& messageName, const LuaTable& parameters);
        void RegisterTableForAgent(const Agent* agent, const nexuslua::LuaTable& table);
    }

    /// \brief base class of the three types of agents
    /// \details These types are AgentCpp, AgentLua and AgentPlugin, but these classes do not need to be part of the public interface.
    class NEXUSLUA_EXPORT Agent
    {
        struct Impl;
        std::unique_ptr<Impl> _impl;

    public:
        Agent(const std::shared_ptr<agents>& agents);
        virtual ~Agent();

        virtual std::string                                GetName() const = 0;                              ///< return the name of the agent
        virtual std::filesystem::path                      GetInstallFolder() const;                         ///< if this agent is installed as a plugin, return its installation folder, otherwise an empty string
        virtual std::filesystem::path                      GetPersistentFolder() const;                      ///< if this agent is installed as a plugin, return the sub folder inside the installation folder that persists during updates, otherwise an empty string
        virtual std::string                                GetVersionOnline() const;                         ///< if this agent is installed as a plugin, return the online version, otherwise an empty string
        virtual std::string                                GetVersionInstalled() const;                      ///< if this agent is installed as a plugin, return its installed version, otherwise an empty string
        virtual bool                                       IsFreeware() const;                               ///< if this agent is installed as a plugin, return if it is freeware, otherwise `false`
        virtual std::string                                GetUrlHelp() const;                               ///< if this agent is installed as a plugin, return the URL that contains help about it, otherwise an empty string
        virtual std::string                                GetUrlDownload() const;                           ///< if this agent is installed as a plugin, return its download URL, otherwise an empty string
        virtual std::string                                GetUrlLicense() const;                            ///< if this agent is installed as a plugin, return an URL with license information, otherwise an empty string
        virtual std::string                                GetUrlPurchase() const;                           ///< if this agent is installed as a plugin, return an URL where you can purchase a license, otherwise an empty string
        virtual std::string                                GetLicensee() const;                              ///< if this agent is installed as a plugin and a license file has been installed, return the licensee, otherwise an empty string
        virtual const std::map<std::string, AgentMessage>& GetMessages() const;                              ///< return a reference to all of the messages this agent supports, using the message name as key
        virtual const AgentMessage&                        GetMessage(const std::string& messageName) const; ///< return the message with the given name that this agent accepts
        int                                                GetId() const;                                    ///< returns a unique ID of this agent

        Configuration&          GetConfiguration();
        std::shared_ptr<agents> GetAgents();

    protected:
        void         Start(const std::filesystem::path& luaPath, const std::string& luaCode);
        void         Start(const CppHandler& cppHandler);
        virtual void AddMessage(const std::string& messageName, const LuaTable::nested_tables& parameterDescriptions, const std::string& displayName, const std::string& description, const std::string& icon);

        std::map<std::string, AgentMessage> _messages;
        friend void ::nexuslua::LuaExtension::AddMessage(Agent* agent, const std::string& luaPath, const std::string& messageName, const LuaTable& parameters);
    };
}
