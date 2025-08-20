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
#include "agent_message.hpp"
#include "agents.hpp"
#include "plugin_spec.hpp"

#include "nexuslua_export.h"

#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace nexuslua
{
    class PluginSpec;

    class NEXUSLUA_EXPORT AgentPlugin : public Agent
    {
        struct Impl;
        std::unique_ptr<Impl> _impl;

    public:
        AgentPlugin(const std::shared_ptr<agents>& agent_group);
        explicit AgentPlugin(const std::shared_ptr<agents>& agent_group, const std::filesystem::path& pluginPath);
        explicit AgentPlugin(const std::shared_ptr<agents>& agent_group, const PluginSpec& pluginSpec);
        ~AgentPlugin() override;

        void Start();

        std::string                                GetName() const override;
        std::filesystem::path                      GetInstallFolder() const override;
        std::filesystem::path                      GetPersistentFolder() const override;
        std::string                                GetVersionOnline() const override;
        std::string                                GetVersionInstalled() const override;
        bool                                       IsFreeware() const override;
        std::string                                GetUrlHelp() const override;
        std::string                                GetUrlDownload() const override;
        std::string                                GetUrlLicense() const override;
        std::string                                GetUrlPurchase() const override;
        std::string                                GetLicensee() const override;
        const std::map<std::string, AgentMessage>& GetMessages() const override;
        const AgentMessage&                        GetMessage(const std::string& messageName) const override;

        const PluginSpec& GetPluginSpec() const;
    };
}
