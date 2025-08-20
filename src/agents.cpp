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

#include "agents.hpp"

#include "agent.hpp"
#include "agent_cpp.hpp"
#include "agent_lua.hpp"
#include "agent_plugin.hpp"
#include "description.hpp"
#include "lua.hpp"
#include "lua_extension.hpp"
#include "message_counter.hpp"
#include "thread_pool.hpp"
#include "utility.hpp"

#include <cbeam/filesystem/io.hpp>
#include <cbeam/filesystem/path.hpp>
#include <cbeam/logging/log_manager.hpp>
#include <cbeam/platform/system_folders.hpp>
#include <cbeam/serialization/xpod.hpp>

#include <string>
#include <thread>

namespace nexuslua
{
    using namespace std::string_literals;

    struct agents::impl
    {
        std::unique_ptr<std::map<std::string, std::shared_ptr<const Agent>>> _plugins{std::make_unique<std::map<std::string, std::shared_ptr<const Agent>>>()}; // TODO integrate into _agents
        std::unique_ptr<std::map<std::string, std::shared_ptr<Agent>>>       _agents{std::make_unique<std::map<std::string, std::shared_ptr<Agent>>>()};
        bool                                                                 _scannedPlugins = false;
    };

    agents::agents()
        : _impl{std::make_unique<agents::impl>()}
    {
    }

    agents::~agents()
    {
        DeleteAgents();
    }

    void agents::DeleteAgents()
    {
        try
        {
            CBEAM_LOG_DEBUG("Destructing all nexuslua agents..."s);
            _impl->_plugins.reset();
            _impl->_agents.reset();
            LuaExtension::DeregisterTablesOfAgents();
            CBEAM_LOG_DEBUG("Destructed all nexuslua agents."s);
        }
        catch (const std::exception& ex)
        {
            CBEAM_LOG("Error during destruction of all nexuslua agents:"s + ex.what());
        }
    }

    const std::map<std::string, std::shared_ptr<const Agent>>& agents::GetPlugins()
    {
        if (!_impl->_scannedPlugins)
        {
            std::filesystem::path plugin_path = cbeam::filesystem::get_user_data_dir() / description::GetProductName() / "plugins";
            CBEAM_LOG_DEBUG("Scanning plugins in " + plugin_path.string());

            cbeam::filesystem::path pluginBaseDir(plugin_path);
            pluginBaseDir.create_directory(false);

            for (auto& pluginPath : pluginBaseDir.get_subdirs())
            {
                auto plugin = std::make_shared<AgentPlugin>(shared_from_this(), pluginPath);

                if (_impl->_plugins->count(plugin->GetName()) == 1)
                {
                    throw std::runtime_error("Agent name " + plugin->GetName() + " is already used by a different agent. This might also be caused by a manually created directory in " + plugin_path.string());
                }

                plugin->Start();
                (*_impl->_plugins)[plugin->GetName()] = plugin;
            }

            _impl->_scannedPlugins = true;
            CBEAM_LOG("Found " + std::to_string(_impl->_plugins->size()) + " plugins in " + plugin_path.string());
        }

        return *_impl->_plugins;
    }

    void agents::InvalidatePluginScan()
    {
        _impl->_scannedPlugins = false;
        _impl->_plugins->clear(); // TODO
    }

    const AgentMessage& agents::GetMessage(const std::string& agentName, const std::string& messageName)
    {
        const auto& itPlugin = GetPlugins().find(agentName);
        if (itPlugin == GetPlugins().end())
        {
            const auto& itAgent = _impl->_agents->find(agentName);

            if (itAgent == _impl->_agents->end())
            {
                throw std::runtime_error("nexuslua::agents::GetMessage: Unknown agent '" + agentName + "'");
            }

            return itAgent->second->GetMessage(messageName);
        }
        else
        {
            return itPlugin->second->GetMessage(messageName);
        }
    }

    void agents::AddMessageForCppAgent(const std::string& agentName, const std::string& messageName)
    {
        auto it = _impl->_agents->find(agentName);

        if (it == _impl->_agents->end())
        {
            throw std::runtime_error("nexuslua::agents::AddMessageForCppAgent: Unknown agent '" + agentName + "'");
        }

        AgentCpp* agentCpp = dynamic_cast<AgentCpp*>(it->second.get());

        if (!agentCpp)
        {
            throw std::runtime_error("nexuslua::agents::AddMessageForCppAgent: Agent '" + agentName + "' is not a C++ agent");
        }

        agentCpp->AddMessage(messageName);
    }

    std::shared_ptr<AgentCpp> agents::Add(const std::string& agentName, const CppHandler& cppHandler, const LuaTable& predefinedTable)
    {
        if (_impl->_agents->count(agentName) == 1)
        {
            throw std::runtime_error("nexuslua::agents: cpp agent '" + agentName + "' already exists.");
        }

        auto agentCpp                = std::make_shared<AgentCpp>(shared_from_this(), agentName);
        (*_impl->_agents)[agentName] = agentCpp;

        LuaExtension::RegisterTableForAgent(agentCpp.get(), predefinedTable);

        agentCpp->Start(cppHandler);
        return agentCpp;
    }

    std::shared_ptr<AgentLua> agents::Add(const std::string& agentName, const std::filesystem::path& pathToLuaFile, const std::string& luaCode, const LuaTable& predefinedTable)
    {
        if (_impl->_agents->count(agentName) == 1)
        {
            throw std::runtime_error("nexuslua::agents: agent '" + agentName + "' already exists.");
        }

        auto agentLua                = std::make_shared<AgentLua>(shared_from_this(), agentName);
        (*_impl->_agents)[agentName] = agentLua;

        LuaExtension::RegisterTableForAgent(agentLua.get(), predefinedTable);

        agentLua->Start(pathToLuaFile, luaCode);
        return agentLua;
    }

    std::shared_ptr<Agent> agents::GetAgent(const std::string& agentName)
    {
        auto it = _impl->_agents->find(agentName);
        return it == _impl->_agents->end() ? nullptr : it->second;
    }

    void agents::WaitUntilMessageQueueIsEmpty()
    {
        CBEAM_LOG("WaitUntilMessageQueueIsEmpty: waiting until message queue is empty.");
        message_counter::get()->wait_until_empty();
        CBEAM_LOG("WaitUntilMessageQueueIsEmpty: detected empty message queue.");
    }

    void agents::ShutdownAgents()
    {
        cbeam::lifecycle::singleton<ThreadPool>::release("nexuslua::thread_pool");
        CBEAM_LOG("ShutdownAgents: detected destruction of all agent threads."s);
    }

    int64_t agents::TotalSizeOfMessagesQueues()
    {
        return nexuslua::message_counter::get()->size();
    }

    PluginInstallResult agents::InstallPlugin(const std::filesystem::path& srcFolder, std::string& errorMessage)
    {
        PluginSpec pluginSpec(srcFolder);
        return InstallPlugin(std::make_shared<AgentPlugin>(shared_from_this(), pluginSpec), srcFolder, errorMessage);
    }

    PluginInstallResult agents::InstallPlugin(const std::shared_ptr<::nexuslua::Agent>& agent, const std::filesystem::path& srcFolder, std::string& errorMessage)
    {
        CBEAM_LOG("agents: Installing plugin from " + srcFolder.string());

        AgentPlugin* agentPlugin = dynamic_cast<AgentPlugin*>(agent.get());

        if (!agentPlugin)
        {
            errorMessage = "Internal error: method InstallPlugin is only meant to install agents of type 'plugin, not other types of agents";
            return PluginInstallResult::ERROR_WHILE_CREATING_INSTANCE;
        }

        if (!std::filesystem::exists(srcFolder))
        {
            return PluginInstallResult::ERROR_DIRECTORY_DOES_NOT_EXIST;
        }

        if (!std::filesystem::is_directory(srcFolder))
        {
            return PluginInstallResult::ERROR_INVALID_SRC;
        }

        std::filesystem::path dstFolder = cbeam::filesystem::get_user_data_dir() / description::GetProductName() / "plugins" / agent->GetName();

        if (std::filesystem::exists(dstFolder))
        {
            return PluginInstallResult::ERROR_PLUGIN_ALREADY_INSTALLED;
        }

        if (_impl->_plugins->count(agent->GetName()) == 1)
        {
            errorMessage = "AgentPlugin " + agent->GetName() + " could not be installed because its name is already in use by a different agent.";
            return PluginInstallResult::ERROR_WHILE_CREATING_INSTANCE;
        }

        try
        {
            cbeam::filesystem::path(srcFolder).copy_to(dstFolder);
        }
        catch (...)
        {
            std::filesystem::remove_all(dstFolder);
            return PluginInstallResult::ERROR_COPYING_PLUGIN_TO_TARGET_DIRECTORY;
        }

        std::shared_ptr<AgentPlugin> plugin = std::make_shared<AgentPlugin>(shared_from_this(), dstFolder);

        if (plugin->GetPluginSpec() != agentPlugin->GetPluginSpec())
        {
            std::filesystem::remove_all(dstFolder);
            errorMessage = "The plugin spec on the server differs from the spec of the plugin that actually was downloaded. This is not expected and could mean a security risk. Installation has been aborted. If you are absolutely sure then you can still install the plugin manually from source directory '" + srcFolder.string() + "'.";
            return PluginInstallResult::ERROR_WHILE_CREATING_INSTANCE;
        }

        (*_impl->_plugins)[plugin->GetName()] = plugin;
        CBEAM_LOG("agents: Successfully installed plugin '" + plugin->GetName() + "' from " + srcFolder.string());
        return PluginInstallResult::SUCCESS;
    }

    PluginUninstallResult agents::UninstallPlugin(const std::string& name)
    {
        CBEAM_LOG("agents: Uninstalling plugin '" + name + "'");
        if (_impl->_plugins->count(name) == 0)
            return {PluginUninstallResult::Result::ERROR_INTERNAL_PLUGIN_DOES_NOT_EXIST, ""};

        const AgentPlugin* agentPlugin = dynamic_cast<const AgentPlugin*>((*_impl->_plugins)[name].get());

        if (!agentPlugin)
        {
            return {PluginUninstallResult::Result::ERROR_INTERNAL_PLUGIN_DOES_NOT_EXIST, ""};
        }

        std::filesystem::path pluginPath = agentPlugin->GetInstallFolder();

        std::filesystem::path backup_dir = cbeam::filesystem::unique_temp_dir();

        try
        {
            cbeam::filesystem::path(pluginPath).copy_to(backup_dir);
            cbeam::filesystem::path(pluginPath).remove();
        }
        catch (...)
        {
            return {PluginUninstallResult::Result::ERROR_PLUGIN_IN_USE, backup_dir};
        }

        _impl->_plugins->erase(_impl->_plugins->find(name)); // TODO: DLLs must be unloaded prior removing the directory. Maybe move it to temp?
        CBEAM_LOG("agents: Successfully uninstalled plugin '" + name + "'");
        return {PluginUninstallResult::Result::SUCCESS, backup_dir};
    }

    void agents::RestorePersistentPluginFolder(const std::shared_ptr<::nexuslua::Agent>& plugin, const std::filesystem::path& srcFolder)
    {
        std::filesystem::path srcPersistentFolder = srcFolder / PluginSpec::NameOfPersistentSubFolder;

        if (std::filesystem::exists(srcPersistentFolder))
        {
            std::error_code ec;
            std::filesystem::remove_all(plugin->GetPersistentFolder(), ec);
            cbeam::filesystem::path(srcPersistentFolder).copy_to(plugin->GetPersistentFolder());
        }
        else if (std::filesystem::exists(srcFolder))
        {
            CBEAM_LOG("RestorePersistentPluginFolder: Backup of plugin is available, but it does not contains a folder 'persistent': " + srcFolder.string());
        }
        else
        {
            throw std::runtime_error("Plugin backup folder is not available: " + srcFolder.string());
        }
    }
}
