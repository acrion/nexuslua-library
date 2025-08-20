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

#include "nexuslua_export.h"
#include <filesystem>
#include <string>

namespace nexuslua
{
    class NEXUSLUA_EXPORT PluginSpec
    {
    public:
        // Constructor for local plugins (reads from disk)
        PluginSpec(const std::filesystem::path& pluginPath);

        // Constructors for online plugins (receive TOML content)
        PluginSpec(const std::string& plugin_spec_toml_content, bool online_source = false);
        PluginSpec(const std::string& plugin_spec_toml_content, std::string resolved_download_url);

        bool operator==(const PluginSpec& rhs) const
        {
            return display_name == rhs.display_name
                && ((version_online == rhs.version_online
                     && version_installed == rhs.version_installed)
                    || (version_online == rhs.version_installed
                        && version_installed == rhs.version_online))
                && is_freeware == rhs.is_freeware
                && url_help == rhs.url_help
                && url_download == rhs.url_download
                && url_license == rhs.url_license
                && url_purchase == rhs.url_purchase;
        }
        bool operator!=(const PluginSpec& rhs) const
        {
            return !(*this == rhs);
        }

        std::filesystem::path GetInstallFolder() const;
        std::filesystem::path GetPersistentFolder() const;
        std::string           GetName() const;
        std::string           GetVersionOnline() const;
        std::string           GetVersionInstalled() const;
        bool                  IsFreeware() const;
        std::string           GetUrlHelp() const;
        std::string           GetUrlDownload() const;
        std::string           GetUrlLicense() const;
        std::string           GetUrlPurchase() const;

        void SetVersionInstalled(const std::string& version);

        static const char* NameOfPersistentSubFolder;

    protected:
        void ValidateInitialization() const;

        std::string display_name;
        std::string version_online;
        std::string version_installed;
        bool        is_freeware{true};
        std::string url_help;
        std::string url_download;
        std::string url_license;
        std::string url_purchase;

    private:
        void init(std::string plugin_spec_toml_content, bool online_source);

        bool _initialized{false};
    };
}