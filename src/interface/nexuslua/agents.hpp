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

#include "cpp_handler.hpp"
#include "lua_table.hpp"
#include "plugin_install_result.hpp"

#include "nexuslua_export.h"

#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace nexuslua
{
    class Agent;
    class AgentCpp;
    class AgentLua;
    class PluginSpec;
    class AgentMessage;

    /// \brief Functions related to agents or plugins, which are (un-)installable agents with meta data like a version, see class nexuslua::Agent
    class NEXUSLUA_EXPORT agents : public std::enable_shared_from_this<agents>
    {
        struct impl;
        std::unique_ptr<impl> _impl;

    public:
        agents();
        virtual ~agents();

        void                                                       DeleteAgents();
        const std::map<std::string, std::shared_ptr<const Agent>>& GetPlugins();                                                                                                                      ///< key is plugin name, same as AgentPlugin::GetName()
        void                                                       InvalidatePluginScan();                                                                                                            ///< after calling this function, the next call to agents::GetPlugins will rescan the plugin directory
        PluginInstallResult                                        InstallPlugin(const std::filesystem::path& srcFolder, std::string& errorMessage);                                                  ///< user selects a directory with a plugin to be installed. This function installs it. In case of ERROR_WHILE_CREATING_INSTANCE, errorMessage will contain an English error description
        PluginInstallResult                                        InstallPlugin(const std::shared_ptr<::nexuslua::Agent>& agent, const std::filesystem::path& srcFolder, std::string& errorMessage); ///< user selects a directory with a plugin to be installed. This function installs it. In case of ERROR_WHILE_CREATING_INSTANCE, errorMessage will contain an English error description
        PluginUninstallResult                                      UninstallPlugin(const std::string& name);                                                                                          ///< the name is identical with AgentPlugin::GetName()
        void                                                       RestorePersistentPluginFolder(const std::shared_ptr<::nexuslua::Agent>& plugin, const std::filesystem::path& srcFolder);           ///< copy the persistent subfolder from the given directory to the plugin folder
        const AgentMessage&                                        GetMessage(const std::string& agentName, const std::string& messageName);                                                          ///< return the given message

        /// \brief creates a new hardware thread that calls cppHandler as soon as a message is sent to it via either nexuslua send, or nexuslua::AgentMessage::Send
        /// @param agentName the name of the agent to be added
        /// @param cppHandler the handler for the agent, which needs to have the signature \link nexuslua::CppHandler CppHandler \endlink
        /// @param predefinedTable can be used to predefine arbitrary Lua tables, e. g. the default table arg containing command line arguments of the Lua executable
        /// @return a shared pointer to the newly added agent
        std::shared_ptr<AgentCpp> Add(const std::string& agentName, const CppHandler& cppHandler, const LuaTable& predefinedTable = {});

        /// \brief creates a new hardware thread that calls luaCode, if not empty, otherwise the given lua file
        /// @param agentName the name of the agent to be added
        /// @param pathToLuaFile either the path of the Lua file that shall be used to handle messages, or the path to the Lua file that contains the luaCode given in the third parameter
        /// @param luaCode optional Lua code as string that shall be used to handle messages to this agent
        /// @param predefinedTable can be used to predefine arbitrary Lua tables, e. g. the default table arg containing command line arguments of the Lua executable
        std::shared_ptr<AgentLua> Add(const std::string& agentName, const std::filesystem::path& pathToLuaFile, const std::string& luaCode = "", const LuaTable& predefinedTable = {});

        /// \brief returns the Agent instance associated with the given agent name, or nullptr if there is no such agent
        /// @param agentName the name of the agent to be added
        /// @returns the Agent instance associated with agentName, or nullptr if there is no such agent
        std::shared_ptr<Agent> GetAgent(const std::string& agentName);

        /// \brief registers a message for an nexuslua::Agent that has been created via nexuslua::agents::Add.
        /// @param agentName the name of the agent for which the message shall be registered
        /// @param messageName the name of the new message
        void           AddMessageForCppAgent(const std::string& agentName, const std::string& messageName);
        void           WaitUntilMessageQueueIsEmpty(); ///< wait until the nexuslua agents processed all remaining messages
        void           ShutdownAgents();   ///< if the main application quits, it should use this function to make sure all threads have ended before the main function returned or the shared library is being unloaded
        static int64_t TotalSizeOfMessagesQueues();
    };
}
