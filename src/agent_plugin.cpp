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

#include "agent_plugin.hpp"

#include "agent.hpp"
#include "agent_thread_lua.hpp"
#include "lua.hpp"
#include "platform_specific.hpp"

#include <cbeam/filesystem/path.hpp>
#include <cbeam/logging/log_manager.hpp>
#include <cbeam/serialization/xpod.hpp>

#include <filesystem>

#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>

namespace nexuslua
{
    using namespace std::string_literals;

    struct AgentPlugin::Impl
    {
        Impl() = default;
        explicit Impl(const std::filesystem::path& pluginPath);
        explicit Impl(const PluginSpec& pluginSpec);

        std::string _licensee;
        PluginSpec  _pluginSpec;
    };

    AgentPlugin::AgentPlugin(const std::shared_ptr<agents>& agent_group)
        : Agent{agent_group}
    {
    }

    AgentPlugin::AgentPlugin(const std::shared_ptr<agents>& agent_group, const std::filesystem::path& pluginPath)
        : Agent{agent_group}
        , _impl{std::make_unique<Impl>(pluginPath)}
    {
    }

    AgentPlugin::AgentPlugin(const std::shared_ptr<agents>& agent_group, const PluginSpec& pluginSpec)
        : Agent{agent_group}
        , _impl{std::make_unique<Impl>(pluginSpec)}
    {
    }

    AgentPlugin::~AgentPlugin()
    {
        CBEAM_LOG_DEBUG("Destroying AgentPlugin '" + GetName() + "'");
    }

    void AgentPlugin::Start()
    {
        Agent::Start(GetInstallFolder() / "main.lua", ""s);
    }

    std::string AgentPlugin::GetName() const
    {
        return _impl->_pluginSpec.GetName();
    }

    std::filesystem::path AgentPlugin::GetInstallFolder() const
    {
        return _impl->_pluginSpec.GetInstallFolder();
    }

    std::filesystem::path AgentPlugin::GetPersistentFolder() const
    {
        return _impl->_pluginSpec.GetPersistentFolder();
    }

    std::string AgentPlugin::GetVersionOnline() const
    {
        return _impl->_pluginSpec.GetVersionOnline();
    }

    std::string AgentPlugin::GetVersionInstalled() const
    {
        return _impl->_pluginSpec.GetVersionInstalled();
    }

    bool AgentPlugin::IsFreeware() const
    {
        return _impl->_pluginSpec.IsFreeware();
    }

    std::string AgentPlugin::GetUrlHelp() const
    {
        return _impl->_pluginSpec.GetUrlHelp();
    }

    std::string AgentPlugin::GetUrlDownload() const
    {
        return _impl->_pluginSpec.GetUrlDownload();
    }

    std::string AgentPlugin::GetUrlLicense() const
    {
        return _impl->_pluginSpec.GetUrlLicense();
    }

    std::string AgentPlugin::GetUrlPurchase() const
    {
        return _impl->_pluginSpec.GetUrlPurchase();
    }

    std::string AgentPlugin::GetLicensee() const
    {
        _impl->_licensee = "todo"; // TODO
        return _impl->_licensee;
    }

    const std::map<std::string, AgentMessage>& AgentPlugin::GetMessages() const
    {
        return Agent::GetMessages();
    }

    const AgentMessage& AgentPlugin::GetMessage(const std::string& messageName) const
    {
        return Agent::GetMessage(messageName);
    }

    const PluginSpec& AgentPlugin::GetPluginSpec() const
    {
        return _impl->_pluginSpec;
    }

    AgentPlugin::Impl::Impl(const std::filesystem::path& pluginPath)
        : _pluginSpec{pluginPath}
    {
        CBEAM_LOG_DEBUG("---- Constructing plugin from directory " + pluginPath.string());
        std::string path1 = _pluginSpec.GetInstallFolder().string();
        std::string path2 = ((std::filesystem::path)cbeam::filesystem::path(pluginPath)).string();

        if (cbeam::filesystem::path(_pluginSpec.GetInstallFolder()) != cbeam::filesystem::path(pluginPath))
        {
            throw std::runtime_error("Plugin directory '" + pluginPath.string() + "' does not match expected plugin directory '" + _pluginSpec.GetInstallFolder().string());
        }

        CBEAM_LOG_DEBUG("---- Finished plugin construction from directory " + pluginPath.string());
    }

    AgentPlugin::Impl::Impl(const PluginSpec& pluginSpec)
        : _pluginSpec{pluginSpec}
    {
    }
}
